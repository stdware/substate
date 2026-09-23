#include "BytesNode.h"

#include <algorithm>
#include <cassert>

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
        action->execute(Action::Execute);
        NodePrivate::pushAction(model(), std::move(action));
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
        action->execute(Action::Execute);
        NodePrivate::pushAction(model(), std::move(action));
    }

    void BytesNode::replace(int index, ArrayView<char> bytes) {
        assert(isWritable());
        assert(index >= 0 && index <= size());

        const int overlap = std::min(int(bytes.size()), size() - index);
        const auto within = bytes.take_front(size_t(overlap));
        const auto beyond = bytes.drop_front(size_t(overlap));

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
            action->execute(Action::Execute);
            NodePrivate::pushAction(model(), std::move(action));
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

    BytesInsDelAction::BytesInsDelAction(int type, BytesNode *parent, int index,
                                         std::vector<char> bytes)
        : Action(type), m_parent(parent), m_index(index), m_bytes(std::move(bytes)) {
    }

    BytesInsDelAction::~BytesInsDelAction() = default;

    void BytesInsDelAction::execute(Operation operation) {
        auto &data = m_parent->m_data;
        if ((type() == BytesInsert) == isForward(operation)) {
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
        const auto &bytes = isForward(operation) ? m_bytes : m_oldBytes;
        std::copy(bytes.begin(), bytes.end(), m_parent->m_data.begin() + m_index);
    }

}
