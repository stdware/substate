// Copyright (C) 2022-2025 Stdware Collections (https://www.github.com/stdware)
// SPDX-License-Identifier: Apache-2.0

#ifndef SUBSTATE_CODEC_H
#define SUBSTATE_CODEC_H

#include <cstdint>
#include <functional>
#include <map>
#include <memory>
#include <utility>
#include <vector>

#include <substate/ArrayView.h>
#include <substate/BinaryStream.h>

namespace ss {

    class Action;

    class Decoder;

    class Model;

    class Node;

    /// The registry of the node types and action types that can be decoded.
    ///
    /// The content of a node is written and read by the node itself, see Node::writeContent() and
    /// Node::readContent(), so that a subclass of a node type inherits its encoding. A node type is
    /// registered with a factory that creates an empty free node of the class for the type. An
    /// action type is registered with a function that reads the action.
    ///
    /// \note A node whose type has no factory, for example an ArrayNode or StructNode with the
    ///       default type, cannot be decoded.
    class SUBSTATE_EXPORT Codec {
    public:
        using NodeFactory = std::function<std::unique_ptr<Node>()>;
        using ActionReader = std::function<std::unique_ptr<Action>(Decoder &)>;

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

    /// Writes nodes and actions to a binary stream.
    ///
    /// A failure is recorded in the state of the stream and remains until the stream is cleared.
    class SUBSTATE_EXPORT Encoder {
    public:
        inline explicit Encoder(OBinaryStream &stream);

        Encoder(const Encoder &) = delete;
        Encoder &operator=(const Encoder &) = delete;

        inline OBinaryStream &stream() const;

        /// Returns whether a write failed, because the stream failed or a value has no encoding.
        inline bool fail() const;

        inline void setFailed();

        /// Writes \a node and its descendants with their types and identifiers, or a null marker
        /// if \a node is \c nullptr.
        void writeNode(const Node *node);

        /// Writes the identifier of \a node, or 0 if \a node is \c nullptr. The node is read back
        /// by Decoder::readReference(), which requires it to exist in the model.
        void writeReference(const Node *node);

        /// Writes the type of \a action and the action in its form before its first execution:
        /// the nodes that the action owns before its first execution, such as the inserted nodes,
        /// are written with their content, and the other nodes as references.
        ///
        /// \note The content of an inserted node is its present content, which equals the content
        ///       at the insertion only until a later action modifies the node. A storage engine
        ///       therefore writes an action when ModelObserver::actionApplied() reports its first
        ///       execution.
        void writeAction(const Action &action);

        /// Writes a byte count and \a bytes.
        void writeBytes(ArrayView<char> bytes);

    private:
        OBinaryStream &m_stream;
    };

    inline Encoder::Encoder(OBinaryStream &stream) : m_stream(stream) {
    }

    inline OBinaryStream &Encoder::stream() const {
        return m_stream;
    }

    inline bool Encoder::fail() const {
        return m_stream.fail();
    }

    inline void Encoder::setFailed() {
        m_stream.setState(std::ios::failbit);
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
        /// references and actions cannot be read.
        Decoder(const Codec &codec, IBinaryStream &stream, Model *model);

        ~Decoder();

        Decoder(const Decoder &) = delete;
        Decoder &operator=(const Decoder &) = delete;

        inline const Codec &codec() const;
        inline IBinaryStream &stream() const;
        inline Model *model() const;

        /// Returns whether a read failed, because the stream failed or the data is invalid.
        inline bool fail() const;

        inline void setFailed();

        /// Reads a node written by Encoder::writeNode(). Returns \c nullptr for a null marker and
        /// on failure. The read fails if a type is not registered, if the content is invalid, or
        /// if an identifier is 0 or exists already in the model.
        std::unique_ptr<Node> readNode();

        /// Reads a reference written by Encoder::writeReference() and returns the node of the
        /// model. Returns \c nullptr for a null reference and on failure. The read fails if the
        /// node does not exist.
        Node *readReference();

        /// Reads an action written by Encoder::writeAction(). Returns \c nullptr on failure. The
        /// action is in its form before its first execution, and is valid only for the tree in
        /// which the action was executed, which the caller ensures.
        std::unique_ptr<Action> readAction();

        /// Reads bytes written by Encoder::writeBytes().
        std::vector<char> readBytes();

    private:
        const Codec &m_codec;
        IBinaryStream &m_stream;
        Model *m_model;

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
        return m_stream;
    }

    inline Model *Decoder::model() const {
        return m_model;
    }

    inline bool Decoder::fail() const {
        return m_stream.fail();
    }

    inline void Decoder::setFailed() {
        m_stream.setState(std::ios::failbit);
    }

}

#endif // SUBSTATE_CODEC_H
