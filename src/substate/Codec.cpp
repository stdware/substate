#include "Codec.h"

#include <algorithm>
#include <cassert>
#include <limits>
#include <sstream>
#include <string>

#include "BytesNode.h"
#include "Model.h"
#include "Node_p.h"
#include "SheetNode.h"
#include "VectorNode.h"

namespace ss {

    namespace {

        // Replaces a stream pointer for the lifetime of the object.
        template <class Stream>
        class StreamSwitch {
        public:
            StreamSwitch(Stream *&current, Stream *replacement)
                : m_current(current), m_previous(current) {
                m_current = replacement;
            }

            ~StreamSwitch() {
                m_current = m_previous;
            }

            StreamSwitch(const StreamSwitch &) = delete;
            StreamSwitch &operator=(const StreamSwitch &) = delete;

        private:
            Stream *&m_current;
            Stream *m_previous;
        };

    }

    Codec::Codec() {
        registerNodeType(Node::Bytes, [] { return std::make_unique<BytesNode>(); });
        registerNodeType(Node::Vector, [] { return std::make_unique<VectorNode>(); });
        registerNodeType(Node::Sheet, [] { return std::make_unique<SheetNode>(); });

        registerActionType(Action::RootChange, &RootChangeAction::read);
        registerActionType(Action::Transfer, &TransferAction::read);
        registerActionType(Action::VectorInsert, [](Decoder &decoder, Action::State state) {
            return VectorInsDelAction::read(decoder, Action::VectorInsert, state);
        });
        registerActionType(Action::VectorRemove, [](Decoder &decoder, Action::State state) {
            return VectorInsDelAction::read(decoder, Action::VectorRemove, state);
        });
        registerActionType(Action::VectorMove, &VectorMoveAction::read);
        registerActionType(Action::SheetInsert, [](Decoder &decoder, Action::State state) {
            return SheetInsDelAction::read(decoder, Action::SheetInsert, state);
        });
        registerActionType(Action::SheetRemove, [](Decoder &decoder, Action::State state) {
            return SheetInsDelAction::read(decoder, Action::SheetRemove, state);
        });
        registerActionType(Action::BytesInsert, [](Decoder &decoder, Action::State state) {
            return BytesInsDelAction::read(decoder, Action::BytesInsert, state);
        });
        registerActionType(Action::BytesRemove, [](Decoder &decoder, Action::State state) {
            return BytesInsDelAction::read(decoder, Action::BytesRemove, state);
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

    NodePool::NodePool() = default;

    NodePool::~NodePool() = default;

    bool NodePool::add(std::unique_ptr<Node> node) {
        if (!node || node->isFree() || node->isAttached() || node->parent() ||
            m_nodes.count(node->id()) > 0) {
            return false;
        }
        const auto id = node->id();
        m_nodes.emplace(id, std::move(node));
        return true;
    }

    std::unique_ptr<Node> NodePool::take(std::uint64_t id) {
        auto it = m_nodes.find(id);
        if (it == m_nodes.end()) {
            return nullptr;
        }
        auto node = std::move(it->second);
        m_nodes.erase(it);
        return node;
    }

    void Encoder::writeNode(const Node *node) {
        *m_stream << bool(node);
        if (!node) {
            return;
        }

        // The content is written to a buffer first, because its size precedes it.
        std::ostringstream buffer(std::ios::binary);
        OBinaryStream content(buffer);
        {
            StreamSwitch<OBinaryStream> guard(m_stream, &content);
            NodePrivate::writeContent(node, *this);
        }
        const auto bytes = buffer.str();
        if (content.fail() || bytes.size() > size_t(std::numeric_limits<int32_t>::max())) {
            setFailed();
            return;
        }
        *m_stream << int32_t(node->type()) << uint64_t(node->id());
        writeBytes(ArrayView<char>(bytes.data(), bytes.size()));
    }

    void Encoder::writeReference(const Node *node) {
        // A free node has no identifier to refer to.
        assert(!node || !node->isFree());
        *m_stream << uint64_t(node ? node->id() : 0);
    }

    void Encoder::writeAction(const Action &action) {
        *m_stream << int32_t(action.type());
        action.write(*this);
    }

    void Encoder::writeBytes(ArrayView<char> bytes) {
        *m_stream << int32_t(bytes.size());
        m_stream->out().write(bytes.data(), std::streamsize(bytes.size()));
    }

    Decoder::Decoder(const Codec &codec, IBinaryStream &stream, Model *model, NodePool *pool)
        : m_codec(codec), m_stream(&stream), m_model(model), m_pool(pool) {
    }

    Decoder::~Decoder() = default;

    std::unique_ptr<Node> Decoder::readNode() {
        bool present = false;
        *m_stream >> present;
        if (fail() || !present) {
            return nullptr;
        }

        int32_t type = 0;
        uint64_t id = 0;
        *m_stream >> type >> id;
        auto bytes = readBytes();
        auto node = fail() ? nullptr : m_codec.createNode(type);
        if (!node || node->type() != type || !node->isFree()) {
            setFailed();
            return nullptr;
        }

        // The content is read from its own stream, which it must fill exactly.
        std::istringstream buffer(std::string(bytes.begin(), bytes.end()), std::ios::binary);
        IBinaryStream content(buffer);
        bool valid = false;
        ++m_depth;
        {
            StreamSwitch<IBinaryStream> guard(m_stream, &content);
            valid = NodePrivate::readContent(node.get(), *this) && !content.fail() &&
                    buffer.peek() == std::char_traits<char>::eof();
        }
        --m_depth;
        if (!valid) {
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

    Node *Decoder::readExistingNode() {
        bool present = false;
        *m_stream >> present;
        if (fail() || !present) {
            return nullptr;
        }

        int32_t type = 0;
        uint64_t id = 0;
        *m_stream >> type >> id;
        readBytes();
        auto node = fail() || !m_model ? nullptr : m_model->nodeById(id);
        if (!node || node->type() != type) {
            setFailed();
            return nullptr;
        }
        return node;
    }

    Node *Decoder::readReference() {
        uint64_t id = 0;
        *m_stream >> id;
        if (fail() || id == 0) {
            return nullptr;
        }
        auto node = m_model ? m_model->nodeById(id) : nullptr;
        if (!node) {
            setFailed();
        }
        return node;
    }

    std::unique_ptr<Node> Decoder::takeReference() {
        uint64_t id = 0;
        *m_stream >> id;
        if (fail() || id == 0) {
            return nullptr;
        }
        auto node = m_pool ? m_pool->take(id) : nullptr;
        if (!node) {
            setFailed();
        }
        return node;
    }

    std::unique_ptr<Action> Decoder::readAction(Action::State state) {
        int32_t type = 0;
        *m_stream >> type;
        auto reader = fail() || !m_model ? nullptr : m_codec.actionReader(type);
        if (!reader) {
            setFailed();
            return nullptr;
        }
        auto action = (*reader)(*this, state);
        if (!action || fail()) {
            setFailed();
            return nullptr;
        }
        return action;
    }

    std::vector<char> Decoder::readBytes() {
        int32_t size = 0;
        *m_stream >> size;
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
            m_stream->in().read(bytes.data() + offset, block);
            if (m_stream->in().gcount() != block) {
                setFailed();
                return {};
            }
        }
        return bytes;
    }

}
