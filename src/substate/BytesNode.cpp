#include "BytesNode.h"

#include <algorithm>
#include <cassert>

#include "Codec.h"
#include "Node_p.h"

namespace ss {

    BytesNode::~BytesNode() = default;

    void BytesNode::insert(int index, ArrayView<char> bytes) {
        assert(isWritable());
        assert(NodePrivate::isValidInsertion(index, size()));

        if (bytes.empty()) {
            return;
        }
        if (isFree()) {
            m_data.insert(m_data.begin() + index, bytes.begin(), bytes.end());
            return;
        }

        std::unique_ptr<BytesInsDelAction> action(new BytesInsDelAction(
            Action::BytesInsert, this, index, std::vector<char>(bytes.begin(), bytes.end())));
        NodePrivate::execute(model(), std::move(action));
    }

    void BytesNode::remove(int index, int count) {
        assert(isWritable());
        assert(NodePrivate::isValidRemoval(index, count, size()));

        auto first = m_data.begin() + index;
        auto last = first + count;
        if (isFree()) {
            m_data.erase(first, last);
            return;
        }

        std::unique_ptr<BytesInsDelAction> action(new BytesInsDelAction(
            Action::BytesRemove, this, index, std::vector<char>(first, last)));
        NodePrivate::execute(model(), std::move(action));
    }

    void BytesNode::replace(int index, ArrayView<char> bytes) {
        assert(isWritable());
        assert(index >= 0 && index <= size());

        const int overlap = std::min(int(bytes.size()), size() - index);
        const auto within = bytes.takeFront(size_t(overlap));
        const auto beyond = bytes.dropFront(size_t(overlap));

        if (isFree()) {
            std::copy(within.begin(), within.end(), m_data.begin() + index);
            m_data.insert(m_data.end(), beyond.begin(), beyond.end());
            return;
        }

        if (!within.empty()) {
            auto first = m_data.begin() + index;
            std::unique_ptr<BytesReplaceAction> action(
                new BytesReplaceAction(this, index, std::vector<char>(within.begin(), within.end()),
                                       std::vector<char>(first, first + overlap)));
            NodePrivate::execute(model(), std::move(action));
        }
        if (!beyond.empty()) {
            insert(size(), beyond);
        }
    }

    std::unique_ptr<Node> BytesNode::clone() const {
        std::unique_ptr<BytesNode> node(new BytesNode());
        node->cloneDataFrom(*this);
        return node;
    }

    void BytesNode::cloneDataFrom(const BytesNode &source) {
        assert(isFree() && m_data.empty());
        m_data = source.m_data;
    }

    void BytesNode::writeContent(Encoder &encoder) const {
        encoder.writeBytes(m_data);
    }

    bool BytesNode::readContent(Decoder &decoder) {
        m_data = decoder.readBytes();
        return !decoder.fail();
    }

    BytesInsDelAction::BytesInsDelAction(int type, BytesNode *parent, int index,
                                         std::vector<char> bytes)
        : Action(type), m_parent(parent), m_index(index), m_bytes(std::move(bytes)) {
    }

    BytesInsDelAction::~BytesInsDelAction() = default;

    void BytesInsDelAction::write(Encoder &encoder) const {
        encoder.writeReference(m_parent);
        encoder.stream() << int32_t(m_index);
        encoder.writeBytes(m_bytes);
    }

    std::unique_ptr<Action> BytesInsDelAction::read(Decoder &decoder, int type, State state) {
        // The action owns no node in either state.
        (void) state;
        auto parent = dynamic_cast<BytesNode *>(decoder.readReference());
        int32_t index = -1;
        decoder.stream() >> index;
        auto bytes = decoder.readBytes();
        if (decoder.fail() || !parent || index < 0 || bytes.empty()) {
            decoder.setFailed();
            return nullptr;
        }
        return std::unique_ptr<Action>(
            new BytesInsDelAction(type, parent, index, std::move(bytes)));
    }

    void BytesInsDelAction::execute(Operation operation) {
        auto &data = m_parent->m_data;
        if (isInsertion(operation)) {
            data.insert(data.begin() + m_index, m_bytes.begin(), m_bytes.end());
        } else {
            auto first = data.begin() + m_index;
            data.erase(first, first + std::ptrdiff_t(m_bytes.size()));
        }
    }

    BytesReplaceAction::BytesReplaceAction(BytesNode *parent, int index, std::vector<char> bytes,
                                           std::vector<char> oldBytes)
        : Action(BytesReplace), m_parent(parent), m_index(index), m_bytes(std::move(bytes)),
          m_oldBytes(std::move(oldBytes)) {
        assert(m_bytes.size() == m_oldBytes.size());
    }

    BytesReplaceAction::~BytesReplaceAction() = default;

    void BytesReplaceAction::execute(Operation operation) {
        const auto replacement = bytes(operation);
        std::copy(replacement.begin(), replacement.end(), m_parent->m_data.begin() + m_index);
    }

    void BytesReplaceAction::write(Encoder &encoder) const {
        encoder.writeReference(m_parent);
        encoder.stream() << int32_t(m_index);
        encoder.writeBytes(m_bytes);
        encoder.writeBytes(m_oldBytes);
    }

    std::unique_ptr<Action> BytesReplaceAction::read(Decoder &decoder, State state) {
        // The action owns no node in either state.
        (void) state;
        auto parent = dynamic_cast<BytesNode *>(decoder.readReference());
        int32_t index = -1;
        decoder.stream() >> index;
        auto bytes = decoder.readBytes();
        auto oldBytes = decoder.readBytes();
        if (decoder.fail() || !parent || index < 0 || bytes.empty() ||
            bytes.size() != oldBytes.size()) {
            decoder.setFailed();
            return nullptr;
        }
        return std::unique_ptr<Action>(
            new BytesReplaceAction(parent, index, std::move(bytes), std::move(oldBytes)));
    }

}
