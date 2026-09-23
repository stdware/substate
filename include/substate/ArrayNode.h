// Copyright (C) 2022-2025 Stdware Collections (https://www.github.com/stdware)
// SPDX-License-Identifier: Apache-2.0

#ifndef SUBSTATE_ARRAYNODE_H
#define SUBSTATE_ARRAYNODE_H

#include <cstring>
#include <type_traits>
#include <vector>

#include <substate/BytesNode.h>

namespace ss {

    /// A BytesNode that holds an array of \a T and addresses it by element index.
    ///
    /// The elements are stored as their bytes in native byte order, and the actions record bytes.
    /// Every index, count and size of this class is in elements. The byte interface of BytesNode
    /// remains available through a reference to the base class.
    ///
    /// \note Elements are read with \c std::memcpy, because the byte array guarantees no alignment
    ///       for \a T.
    template <class T>
    class ArrayNode : public BytesNode {
        static_assert(std::is_trivially_copyable_v<T>,
                      "ArrayNode requires a trivially copyable type");

    public:
        inline ArrayNode();
        inline explicit ArrayNode(int type);

        inline int size() const;
        inline T at(int index) const;
        inline std::vector<T> values() const;

        inline void insert(int index, ArrayView<T> values);
        inline void append(ArrayView<T> values);
        inline void remove(int index, int count);

        /// Overwrites the elements starting at \a index, extending the array if \a values reaches
        /// beyond its end. See BytesNode::replace().
        inline void replace(int index, ArrayView<T> values);

        inline void truncate(int size);
        inline void clear();

        inline std::unique_ptr<Node> clone() const override;

    private:
        static inline ArrayView<char> bytesOf(ArrayView<T> values);
    };

    template <class T>
    inline ArrayNode<T>::ArrayNode() : ArrayNode(Bytes) {
    }

    template <class T>
    inline ArrayNode<T>::ArrayNode(int type) : BytesNode(type) {
    }

    template <class T>
    inline int ArrayNode<T>::size() const {
        return BytesNode::size() / int(sizeof(T));
    }

    template <class T>
    inline T ArrayNode<T>::at(int index) const {
        T value;
        std::memcpy(&value, data().data() + size_t(index) * sizeof(T), sizeof(T));
        return value;
    }

    template <class T>
    inline std::vector<T> ArrayNode<T>::values() const {
        std::vector<T> result((size_t(size())));
        if (!result.empty()) {
            std::memcpy(result.data(), data().data(), result.size() * sizeof(T));
        }
        return result;
    }

    template <class T>
    inline void ArrayNode<T>::insert(int index, ArrayView<T> values) {
        BytesNode::insert(index * int(sizeof(T)), bytesOf(values));
    }

    template <class T>
    inline void ArrayNode<T>::append(ArrayView<T> values) {
        insert(size(), values);
    }

    template <class T>
    inline void ArrayNode<T>::remove(int index, int count) {
        BytesNode::remove(index * int(sizeof(T)), count * int(sizeof(T)));
    }

    template <class T>
    inline void ArrayNode<T>::replace(int index, ArrayView<T> values) {
        BytesNode::replace(index * int(sizeof(T)), bytesOf(values));
    }

    template <class T>
    inline void ArrayNode<T>::truncate(int size) {
        BytesNode::truncate(size * int(sizeof(T)));
    }

    template <class T>
    inline void ArrayNode<T>::clear() {
        BytesNode::clear();
    }

    template <class T>
    inline std::unique_ptr<Node> ArrayNode<T>::clone() const {
        auto node = std::make_unique<ArrayNode<T>>(type());
        node->cloneDataFrom(*this);
        return node;
    }

    template <class T>
    inline ArrayView<char> ArrayNode<T>::bytesOf(ArrayView<T> values) {
        return ArrayView<char>(reinterpret_cast<const char *>(values.data()),
                               values.size() * sizeof(T));
    }

}

#endif // SUBSTATE_ARRAYNODE_H
