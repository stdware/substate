// Copyright (C) 2022-2025 Stdware Collections (https://www.github.com/stdware)
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
        /// beyond its end as an insertion, because a replacement preserves the length.
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

    protected:
        void execute(Operation operation) override;

    private:
        BytesInsDelAction(int type, BytesNode *parent, int index, std::vector<char> bytes);

        BytesNode *m_parent;
        int m_index;
        std::vector<char> m_bytes;

        friend class BytesNode;
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

    /// Replacement of bytes in a BytesNode with bytes of the same length. Owns no node.
    class SUBSTATE_EXPORT BytesReplaceAction : public Action {
    public:
        ~BytesReplaceAction();

        inline BytesNode *parent() const;
        inline int index() const;

        /// The bytes after the replacement.
        inline ArrayView<char> bytes() const;

        /// The bytes before the replacement.
        inline ArrayView<char> oldBytes() const;

    protected:
        void execute(Operation operation) override;

    private:
        BytesReplaceAction(BytesNode *parent, int index, std::vector<char> bytes,
                           std::vector<char> oldBytes);

        BytesNode *m_parent;
        int m_index;
        std::vector<char> m_bytes;
        std::vector<char> m_oldBytes;

        friend class BytesNode;
    };

    inline BytesNode *BytesReplaceAction::parent() const {
        return m_parent;
    }

    inline int BytesReplaceAction::index() const {
        return m_index;
    }

    inline ArrayView<char> BytesReplaceAction::bytes() const {
        return m_bytes;
    }

    inline ArrayView<char> BytesReplaceAction::oldBytes() const {
        return m_oldBytes;
    }

}

#endif // SUBSTATE_BYTESNODE_H
