// Copyright (C) 2022-present Stdware Collections (https://www.github.com/stdware)
// SPDX-License-Identifier: Apache-2.0

#ifndef SUBSTATE_CODEC_H
#define SUBSTATE_CODEC_H

#include <cstddef>
#include <cstdint>
#include <functional>
#include <map>
#include <memory>
#include <utility>
#include <vector>

#include <substate/Action.h>
#include <substate/ArrayView.h>
#include <substate/BinaryStream.h>

namespace ss {

    class Decoder;

    class Model;

    class Node;

    /// The registry of the node types and action types that can be decoded.
    ///
    /// The content of a node is written and read by the node itself, see Node::writeContent() and
    /// Node::readContent(), so that a subclass of a node type inherits its encoding. A node type is
    /// registered with a factory that creates an empty free node of the class for the type. An
    /// action type is registered with a function that reads the action in a given state.
    ///
    /// \note A node whose type has no factory, for example an ArrayNode or StructNode with the
    ///       default type, cannot be decoded.
    class SUBSTATE_EXPORT Codec {
    public:
        using NodeFactory = std::function<std::unique_ptr<Node>()>;
        using ActionReader = std::function<std::unique_ptr<Action>(Decoder &, Action::State)>;

        /// Creates a codec for VectorNode, SheetNode and BytesNode with their default types, and
        /// for the actions of substate.
        Codec();

        virtual ~Codec();

        /// Registers \a factory for nodes of \a type, replacing a previous registration.
        void registerNodeType(int type, NodeFactory factory);

        /// Registers \a reader for actions of \a type, replacing a previous registration.
        void registerActionType(int type, ActionReader reader);

        /// Returns an empty free node of \a type, or \c nullptr if \a type is not registered.
        std::unique_ptr<Node> createNode(int type) const;

        /// Returns the reader of actions of \a type, or \c nullptr if \a type is not registered.
        const ActionReader *actionReader(int type) const;

    private:
        std::map<int, NodeFactory> m_nodeFactories;
        std::map<int, ActionReader> m_actionReaders;
    };

    /// Decoded nodes that are neither in the tree of their model nor owned by an action, until a
    /// decoded action takes them over.
    ///
    /// A storage engine that restores a history decodes the nodes that the applied actions own,
    /// which it records in a checkpoint, into a pool, and decodes the applied actions with the
    /// pool. Each of these actions takes its nodes from the pool, so that every node has exactly
    /// one owner. The pool is empty afterwards if the checkpoint and the actions agree.
    ///
    /// \note The nodes remaining in a pool are destroyed with it, which must precede the
    ///       destruction of their model.
    class SUBSTATE_EXPORT NodePool {
    public:
        NodePool();
        ~NodePool();

        NodePool(const NodePool &) = delete;
        NodePool &operator=(const NodePool &) = delete;

        /// Adds \a node, the root of a subtree decoded into a model.
        ///
        /// \return whether the node was added. It is not, and is destroyed, if it is \c nullptr,
        ///         free, in the tree, has a parent, or has the identifier of a node in the pool.
        bool add(std::unique_ptr<Node> node);

        /// Removes the node with \a id from the pool and returns it, or returns \c nullptr if the
        /// pool has no such node.
        std::unique_ptr<Node> take(std::uint64_t id);

        inline bool empty() const;
        inline std::size_t size() const;

    private:
        std::map<std::uint64_t, std::unique_ptr<Node>> m_nodes;
    };

    inline bool NodePool::empty() const {
        return m_nodes.empty();
    }

    inline std::size_t NodePool::size() const {
        return m_nodes.size();
    }

    /// Writes nodes and actions to a binary stream.
    ///
    /// A failure is recorded in the state of the stream and remains until the stream is cleared.
    class SUBSTATE_EXPORT Encoder {
    public:
        inline explicit Encoder(OBinaryStream &stream);

        Encoder(const Encoder &) = delete;
        Encoder &operator=(const Encoder &) = delete;

        /// The stream to write to, which is a temporary stream while the content of a node is
        /// written.
        inline OBinaryStream &stream() const;

        /// Returns whether a write failed, because the stream failed or a value has no encoding.
        inline bool fail() const;

        inline void setFailed();

        /// Writes \a node and its descendants with their types and identifiers, or a null marker
        /// if \a node is \c nullptr. The content of each node is preceded by its size, so that
        /// Decoder::readExistingNode() can skip it.
        void writeNode(const Node *node);

        /// Writes the identifier of \a node, or 0 if \a node is \c nullptr.
        void writeReference(const Node *node);

        /// Writes the type of \a action and the action.
        ///
        /// The nodes that the action owns while it is unapplied, such as the inserted nodes, are
        /// written with Encoder::writeNode(), and the other nodes with Encoder::writeReference().
        /// The form is the same for both states, and Decoder::readAction() interprets it for the
        /// state of the action.
        ///
        /// \note The content of an inserted node is its present content, which equals the content
        ///       at the insertion only until a later action modifies the node. A storage engine
        ///       therefore writes an action when ModelObserver::actionApplied() reports its first
        ///       execution.
        void writeAction(const Action &action);

        /// Writes a byte count and \a bytes.
        void writeBytes(ArrayView<char> bytes);

    private:
        OBinaryStream *m_stream;
    };

    inline Encoder::Encoder(OBinaryStream &stream) : m_stream(&stream) {
    }

    inline OBinaryStream &Encoder::stream() const {
        return *m_stream;
    }

    inline bool Encoder::fail() const {
        return m_stream->fail();
    }

    inline void Encoder::setFailed() {
        m_stream->setState(std::ios::failbit);
    }

    /// Reads nodes and actions from a binary stream written by an Encoder.
    ///
    /// A failure is recorded in the state of the stream and remains until the stream is cleared.
    /// After a failure, every read returns an empty value, and the nodes decoded partially are
    /// destroyed.
    class SUBSTATE_EXPORT Decoder {
    public:
        /// Creates a decoder for the node types and action types of \a codec.
        ///
        /// The decoded nodes enter \a model with their recorded identifiers without entering its
        /// tree, and the identifier counter of the model is raised above them. If \a model is
        /// \c nullptr, the nodes are decoded as free nodes, the identifiers are ignored, and
        /// references and actions cannot be read. Applied actions take the nodes they own from
        /// \a pool, see NodePool.
        Decoder(const Codec &codec, IBinaryStream &stream, Model *model, NodePool *pool = nullptr);

        ~Decoder();

        Decoder(const Decoder &) = delete;
        Decoder &operator=(const Decoder &) = delete;

        inline const Codec &codec() const;

        /// The stream to read from, which is a temporary stream while the content of a node is
        /// read.
        inline IBinaryStream &stream() const;

        inline Model *model() const;
        inline NodePool *pool() const;

        /// Returns whether a read failed, because the stream failed or the data is invalid.
        inline bool fail() const;

        inline void setFailed();

        /// Reads a node written by Encoder::writeNode() and creates it with its descendants.
        /// Returns \c nullptr for a null marker and on failure. The read fails if a type is not
        /// registered, if the content is invalid, or if an identifier is 0 or exists already in
        /// the model.
        std::unique_ptr<Node> readNode();

        /// Reads a node written by Encoder::writeNode() without its content, and returns the node
        /// of the model with its identifier. Returns \c nullptr for a null marker and on failure.
        /// The read fails if the model has no such node, or if its type differs.
        Node *readExistingNode();

        /// Reads a reference written by Encoder::writeReference() and returns the node of the
        /// model. Returns \c nullptr for a null reference and on failure. The read fails if the
        /// node does not exist.
        Node *readReference();

        /// Reads a reference written by Encoder::writeReference() and takes the node from the
        /// pool. Returns \c nullptr for a null reference and on failure. The read fails if the
        /// decoder has no pool or the pool has no such node.
        std::unique_ptr<Node> takeReference();

        /// Reads an action written by Encoder::writeAction() in \a state. Returns \c nullptr on
        /// failure.
        ///
        /// An unapplied action creates the nodes it owns from their content. An applied action
        /// refers to the nodes that it inserted, which exist in the model, and takes the nodes
        /// that it removed from the pool. The action is valid only for a tree in which it has
        /// \a state, which the caller ensures.
        std::unique_ptr<Action> readAction(Action::State state = Action::Unapplied);

        /// Reads bytes written by Encoder::writeBytes().
        std::vector<char> readBytes();

    private:
        const Codec &m_codec;
        IBinaryStream *m_stream;
        Model *m_model;
        NodePool *m_pool;

        // The depth of nested readNode() calls, and the nodes of the subtree being read with
        // their recorded identifiers. The identifiers are assigned when the outermost read
        // completes, because a node can be inserted into its free parent only while it is free.
        int m_depth = 0;
        std::vector<std::pair<Node *, std::uint64_t>> m_pending;
    };

    inline const Codec &Decoder::codec() const {
        return m_codec;
    }

    inline IBinaryStream &Decoder::stream() const {
        return *m_stream;
    }

    inline Model *Decoder::model() const {
        return m_model;
    }

    inline NodePool *Decoder::pool() const {
        return m_pool;
    }

    inline bool Decoder::fail() const {
        return m_stream->fail();
    }

    inline void Decoder::setFailed() {
        m_stream->setState(std::ios::failbit);
    }

}

#endif // SUBSTATE_CODEC_H
