// Copyright (C) 2022-present Stdware Collections (https://www.github.com/stdware)
// SPDX-License-Identifier: Apache-2.0

#ifndef SUBSTATE_BYTESNODE_H
#define SUBSTATE_BYTESNODE_H

#include <vector>

#include <substate/Action.h>
#include <substate/ArrayView.h>
#include <substate/Node.h>

namespace ss {

    /// A node that holds a contiguous byte array and has no children.
    ///
    /// Arrays of other element types are stored through ArrayNode, which addresses the same
    /// bytes by element index.
    class SUBSTATE_EXPORT BytesNode : public Node {
    public:
        inline BytesNode();
        ~BytesNode();

        inline int size() const;
        inline ArrayView<char> data() const;

        /// Inserts \a bytes before \a index.
        void insert(int index, ArrayView<char> bytes);
        inline void append(ArrayView<char> bytes);

        /// Removes \a count bytes starting at \a index.
        void remove(int index, int count);

        /// Overwrites the bytes starting at \a index with \a bytes, extending the array if
        /// \a bytes reaches beyond its end. \a index must not exceed size().
        ///
        /// In a model, the part within the array is recorded as a replacement and the part
        /// beyond its end as an insertion, because a replacement preserves the length. A part
        /// within the array equal to the current bytes creates no action.
        void replace(int index, ArrayView<char> bytes);

        /// Removes the bytes from \a size to the end.
        inline void truncate(int size);

        inline void clear();

        std::unique_ptr<Node> clone() const override;

    protected:
        inline explicit BytesNode(int type);

        /// Copies the bytes of \a source, for the clone() of a subclass. This node must be free
        /// and empty.
        void cloneDataFrom(const BytesNode &source);

        void writeContent(Encoder &encoder) const override;
        bool readContent(Decoder &decoder) override;

    private:
        std::vector<char> m_data;

        friend class BytesInsDelAction;
        friend class BytesReplaceAction;
    };

    inline BytesNode::BytesNode() : BytesNode(Bytes) {
    }

    inline BytesNode::BytesNode(int type) : Node(type) {
    }

    inline int BytesNode::size() const {
        return int(m_data.size());
    }

    inline ArrayView<char> BytesNode::data() const {
        return m_data;
    }

    inline void BytesNode::append(ArrayView<char> bytes) {
        insert(size(), bytes);
    }

    inline void BytesNode::truncate(int size) {
        if (size < this->size()) {
            remove(size, this->size() - size);
        }
    }

    inline void BytesNode::clear() {
        truncate(0);
    }

    /// Insertion into or removal from a BytesNode. Owns no node.
    class SUBSTATE_EXPORT BytesInsDelAction : public Action {
    public:
        ~BytesInsDelAction();

        inline BytesNode *parent() const;
        inline int index() const;

        /// The inserted or removed bytes.
        inline ArrayView<char> bytes() const;

        /// Returns whether applying the action for \a operation inserts the bytes rather than
        /// removing them.
        inline bool isInsertion(Operation operation = Execute) const;

    protected:
        void execute(Operation operation) override;
        void write(Encoder &encoder) const override;

    private:
        BytesInsDelAction(int type, BytesNode *parent, int index, std::vector<char> bytes);

        static std::unique_ptr<Action> read(Decoder &decoder, int type, State state);

        BytesNode *m_parent;
        int m_index;
        std::vector<char> m_bytes;

        friend class BytesNode;
        friend class Codec;
    };

    inline BytesNode *BytesInsDelAction::parent() const {
        return m_parent;
    }

    inline int BytesInsDelAction::index() const {
        return m_index;
    }

    inline ArrayView<char> BytesInsDelAction::bytes() const {
        return m_bytes;
    }

    inline bool BytesInsDelAction::isInsertion(Operation operation) const {
        return (type() == BytesInsert) == isForward(operation);
    }

    /// Replacement of bytes in a BytesNode with bytes of the same length. Owns no node.
    class SUBSTATE_EXPORT BytesReplaceAction : public Action {
    public:
        ~BytesReplaceAction();

        inline BytesNode *parent() const;
        inline int index() const;

        /// The bytes after the action is applied for \a operation.
        inline ArrayView<char> bytes(Operation operation = Execute) const;

        /// The bytes before the action is applied for \a operation.
        inline ArrayView<char> oldBytes(Operation operation = Execute) const;

    protected:
        void execute(Operation operation) override;
        void write(Encoder &encoder) const override;

    private:
        BytesReplaceAction(BytesNode *parent, int index, std::vector<char> bytes,
                           std::vector<char> oldBytes);

        static std::unique_ptr<Action> read(Decoder &decoder, State state);

        BytesNode *m_parent;
        int m_index;
        std::vector<char> m_bytes;
        std::vector<char> m_oldBytes;

        friend class BytesNode;
        friend class Codec;
    };

    inline BytesNode *BytesReplaceAction::parent() const {
        return m_parent;
    }

    inline int BytesReplaceAction::index() const {
        return m_index;
    }

    inline ArrayView<char> BytesReplaceAction::bytes(Operation operation) const {
        return isForward(operation) ? m_bytes : m_oldBytes;
    }

    inline ArrayView<char> BytesReplaceAction::oldBytes(Operation operation) const {
        return isForward(operation) ? m_oldBytes : m_bytes;
    }

}

#endif // SUBSTATE_BYTESNODE_H
