#include "Codec.h"

#include <algorithm>
#include <cassert>

#include "BytesNode.h"
#include "Model.h"
#include "Node_p.h"
#include "SheetNode.h"
#include "VectorNode.h"

namespace ss {

    Codec::Codec() {
        registerNodeType(Node::Bytes, [] { return std::make_unique<BytesNode>(); });
        registerNodeType(Node::Vector, [] { return std::make_unique<VectorNode>(); });
        registerNodeType(Node::Sheet, [] { return std::make_unique<SheetNode>(); });

        registerActionType(Action::RootChange, &RootChangeAction::read);
        registerActionType(Action::Transfer, &TransferAction::read);
        registerActionType(Action::VectorInsert, [](Decoder &decoder) {
            return VectorInsDelAction::read(decoder, Action::VectorInsert);
        });
        registerActionType(Action::VectorRemove, [](Decoder &decoder) {
            return VectorInsDelAction::read(decoder, Action::VectorRemove);
        });
        registerActionType(Action::VectorMove, &VectorMoveAction::read);
        registerActionType(Action::SheetInsert, [](Decoder &decoder) {
            return SheetInsDelAction::read(decoder, Action::SheetInsert);
        });
        registerActionType(Action::SheetRemove, [](Decoder &decoder) {
            return SheetInsDelAction::read(decoder, Action::SheetRemove);
        });
        registerActionType(Action::BytesInsert, [](Decoder &decoder) {
            return BytesInsDelAction::read(decoder, Action::BytesInsert);
        });
        registerActionType(Action::BytesRemove, [](Decoder &decoder) {
            return BytesInsDelAction::read(decoder, Action::BytesRemove);
        });
        registerActionType(Action::BytesReplace, &BytesReplaceAction::read);
    }

    Codec::~Codec() = default;

    void Codec::registerNodeType(int type, NodeFactory factory) {
        m_nodeFactories[type] = std::move(factory);
    }

    void Codec::registerActionType(int type, ActionReader reader) {
        m_actionReaders[type] = std::move(reader);
    }

    std::unique_ptr<Node> Codec::createNode(int type) const {
        auto it = m_nodeFactories.find(type);
        return it == m_nodeFactories.end() ? nullptr : it->second();
    }

    const Codec::ActionReader *Codec::actionReader(int type) const {
        auto it = m_actionReaders.find(type);
        return it == m_actionReaders.end() ? nullptr : &it->second;
    }

    void Encoder::writeNode(const Node *node) {
        m_stream << bool(node);
        if (!node) {
            return;
        }
        m_stream << int32_t(node->type()) << uint64_t(node->id());
        NodePrivate::writeContent(node, *this);
    }

    void Encoder::writeReference(const Node *node) {
        // A free node has no identifier to refer to.
        assert(!node || !node->isFree());
        m_stream << uint64_t(node ? node->id() : 0);
    }

    void Encoder::writeAction(const Action &action) {
        m_stream << int32_t(action.type());
        action.write(*this);
    }

    void Encoder::writeBytes(ArrayView<char> bytes) {
        m_stream << int32_t(bytes.size());
        m_stream.out().write(bytes.data(), std::streamsize(bytes.size()));
    }

    Decoder::Decoder(const Codec &codec, IBinaryStream &stream, Model *model)
        : m_codec(codec), m_stream(stream), m_model(model) {
    }

    Decoder::~Decoder() = default;

    std::unique_ptr<Node> Decoder::readNode() {
        bool present = false;
        m_stream >> present;
        if (fail() || !present) {
            return nullptr;
        }

        int32_t type = 0;
        uint64_t id = 0;
        m_stream >> type >> id;
        auto node = fail() ? nullptr : m_codec.createNode(type);
        if (!node || node->type() != type || !node->isFree()) {
            setFailed();
            return nullptr;
        }

        ++m_depth;
        const bool valid = NodePrivate::readContent(node.get(), *this);
        --m_depth;
        if (!valid || fail()) {
            setFailed();
            // The nodes of the subtree are destroyed, including the pending ones.
            if (m_depth == 0) {
                m_pending.clear();
            }
            return nullptr;
        }

        if (m_model) {
            m_pending.emplace_back(node.get(), id);
        }
        if (m_depth == 0 && m_model) {
            const auto pending = std::move(m_pending);
            m_pending.clear();
            if (!NodePrivate::adopt(m_model, pending)) {
                setFailed();
                return nullptr;
            }
        }
        return node;
    }

    Node *Decoder::readReference() {
        uint64_t id = 0;
        m_stream >> id;
        if (fail() || id == 0) {
            return nullptr;
        }
        auto node = m_model ? m_model->nodeById(id) : nullptr;
        if (!node) {
            setFailed();
        }
        return node;
    }

    std::unique_ptr<Action> Decoder::readAction() {
        int32_t type = 0;
        m_stream >> type;
        auto reader = fail() || !m_model ? nullptr : m_codec.actionReader(type);
        if (!reader) {
            setFailed();
            return nullptr;
        }
        auto action = (*reader)(*this);
        if (!action || fail()) {
            setFailed();
            return nullptr;
        }
        return action;
    }

    std::vector<char> Decoder::readBytes() {
        int32_t size = 0;
        m_stream >> size;
        if (fail() || size < 0) {
            setFailed();
            return {};
        }
        // Read in blocks, so that a corrupt size fails at the end of the stream rather than
        // allocating its full amount first.
        constexpr int32_t blockSize = 65536;
        std::vector<char> bytes;
        while (int32_t(bytes.size()) < size) {
            const auto offset = bytes.size();
            const auto block = std::min(blockSize, size - int32_t(offset));
            bytes.resize(offset + size_t(block));
            m_stream.in().read(bytes.data() + offset, block);
            if (m_stream.in().gcount() != block) {
                setFailed();
                return {};
            }
        }
        return bytes;
    }

}
