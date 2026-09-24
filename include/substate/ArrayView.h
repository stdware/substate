// Copyright (C) 2022-present Stdware Collections (https://www.github.com/stdware)
// SPDX-License-Identifier: Apache-2.0

#ifndef SUBSTATE_ARRAYVIEW_H
#define SUBSTATE_ARRAYVIEW_H

#include <algorithm>
#include <array>
#include <cassert>
#include <cstddef>
#include <initializer_list>
#include <iterator>
#include <optional>
#include <type_traits>
#include <vector>

namespace ss {

    /// A read-only view of a contiguous array, which does not own the elements. Ported from
    /// \c llvm::ArrayRef.
    ///
    /// The type names and the functions for iteration follow the standard containers, so that a
    /// view works with the standard algorithms and range-based loops.
    ///
    /// \warning A view does not extend the lifetime of the viewed elements, including the
    ///          elements of an initializer list.
    template <class T>
    class ArrayView {
    public:
        using value_type = T;
        using pointer = value_type *;
        using const_pointer = const value_type *;
        using reference = value_type &;
        using const_reference = const value_type &;
        using iterator = const_pointer;
        using const_iterator = const_pointer;
        using reverse_iterator = std::reverse_iterator<iterator>;
        using const_reverse_iterator = std::reverse_iterator<const_iterator>;
        using size_type = std::size_t;
        using difference_type = std::ptrdiff_t;

        inline ArrayView() = default;

        inline ArrayView(std::nullopt_t) {
        }

        /// Creates a view of the single element \a item.
        inline ArrayView(const T &item) : m_data(&item), m_size(1) {
        }

        inline constexpr ArrayView(const T *data, std::size_t length)
            : m_data(data), m_size(length) {
        }

        inline constexpr ArrayView(const T *begin, const T *end)
            : m_data(begin), m_size(std::size_t(end - begin)) {
            assert(begin <= end);
        }

        /// Creates a view of the elements of a container with contiguous storage, such as
        /// \c std::vector.
        template <template <class, class...> class V, class... A>
        inline ArrayView(const V<T, A...> &container)
            : m_data(container.data()), m_size(container.size()) {
        }

        template <std::size_t N>
        inline constexpr ArrayView(const std::array<T, N> &array)
            : m_data(array.data()), m_size(N) {
        }

        template <std::size_t N>
        inline constexpr ArrayView(const T (&array)[N]) : m_data(array), m_size(N) {
        }

#if defined(__GNUC__) && __GNUC__ >= 9
// GCC warns at every use of this constructor that the initializer list is not extended in
// lifetime. The class description states that a view never extends the lifetime of its elements.
#  pragma GCC diagnostic push
#  pragma GCC diagnostic ignored "-Winit-list-lifetime"
#endif
        inline constexpr ArrayView(std::initializer_list<T> list)
            : m_data(list.begin() == list.end() ? nullptr : list.begin()), m_size(list.size()) {
        }
#if defined(__GNUC__) && __GNUC__ >= 9
#  pragma GCC diagnostic pop
#endif

        /// Creates a view of pointers to \c const from a view of pointers.
        template <class U>
        inline ArrayView(
            const ArrayView<U *> &RHS,
            std::enable_if_t<std::is_convertible<U *const *, T const *>::value> * = nullptr)
            : m_data(RHS.data()), m_size(RHS.size()) {
        }

        inline iterator begin() const {
            return m_data;
        }

        inline iterator end() const {
            return m_data + m_size;
        }

        inline reverse_iterator rbegin() const {
            return reverse_iterator(end());
        }

        inline reverse_iterator rend() const {
            return reverse_iterator(begin());
        }

        inline bool empty() const {
            return m_size == 0;
        }

        inline const T *data() const {
            return m_data;
        }

        inline std::size_t size() const {
            return m_size;
        }

        inline const T &front() const {
            assert(!empty());
            return m_data[0];
        }

        inline const T &back() const {
            assert(!empty());
            return m_data[m_size - 1];
        }

        /// Returns whether both views have equal elements.
        inline bool equals(const ArrayView &RHS) const {
            return m_size == RHS.m_size && std::equal(begin(), end(), RHS.begin());
        }

        /// Returns the \a length elements starting at \a start, which must lie within the view.
        inline ArrayView<T> slice(std::size_t start, std::size_t length) const {
            assert(start + length <= size());
            return ArrayView<T>(data() + start, length);
        }

        /// Returns the elements from \a start to the end.
        inline ArrayView<T> slice(std::size_t start) const {
            return dropFront(start);
        }

        /// Returns the view without its first \a count elements, which must exist.
        inline ArrayView<T> dropFront(std::size_t count = 1) const {
            assert(count <= size());
            return slice(count, size() - count);
        }

        /// Returns the view without its last \a count elements, which must exist.
        inline ArrayView<T> dropBack(std::size_t count = 1) const {
            assert(count <= size());
            return slice(0, size() - count);
        }

        /// Returns the first \a count elements, or the whole view if it has fewer.
        inline ArrayView<T> takeFront(std::size_t count = 1) const {
            if (count >= size()) {
                return *this;
            }
            return dropBack(size() - count);
        }

        /// Returns the last \a count elements, or the whole view if it has fewer.
        inline ArrayView<T> takeBack(std::size_t count = 1) const {
            if (count >= size()) {
                return *this;
            }
            return dropFront(size() - count);
        }

        inline const T &operator[](std::size_t index) const {
            assert(index < m_size);
            return m_data[index];
        }

        /// Prevents the assignment of a temporary, whose elements would not outlive the view.
        /// The template form keeps <tt>view = {}</tt> selecting the move assignment operator.
        template <class U>
        std::enable_if_t<std::is_same<U, T>::value, ArrayView<T>> &
            operator=(U &&temporary) = delete;

        /// Prevents the assignment of an initializer list, whose elements would not outlive the
        /// view.
        template <class U>
        std::enable_if_t<std::is_same<U, T>::value, ArrayView<T>> &
            operator=(std::initializer_list<U>) = delete;

        /// Returns a copy of the elements.
        inline std::vector<T> vec() const {
            return std::vector<T>(m_data, m_data + m_size);
        }

    private:
        const T *m_data = nullptr;
        size_type m_size = 0;
    };

    template <class T>
    inline bool operator==(ArrayView<T> LHS, ArrayView<T> RHS) {
        return LHS.equals(RHS);
    }

    template <template <class, class...> class V, class T, class... A>
    inline bool operator==(const V<T, A...> &LHS, ArrayView<T> RHS) {
        return ArrayView<T>(LHS).equals(RHS);
    }

    template <class T>
    inline bool operator!=(ArrayView<T> LHS, ArrayView<T> RHS) {
        return !(LHS == RHS);
    }

    template <template <class, class...> class V, class T, class... A>
    inline bool operator!=(const V<T, A...> &LHS, ArrayView<T> RHS) {
        return !(LHS == RHS);
    }

}

#endif // SUBSTATE_ARRAYVIEW_H
