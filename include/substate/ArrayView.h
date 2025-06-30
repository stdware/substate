#ifndef SUBSTATE_ARRAYVIEW_H
#define SUBSTATE_ARRAYVIEW_H

#include <array>
#include <optional>
#include <vector>
#include <cassert>

namespace ss {

    /// ArrayView - A lightweight view of an array or vector, ported from \c llvm::ArrayRef.
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
        using size_type = size_t;
        using difference_type = ptrdiff_t;

    public:
        ArrayView() = default;

        ArrayView(std::nullopt_t) {
        }

        ArrayView(const T &item) : _data(&item), _size(1) {
        }

        constexpr ArrayView(const T *data, size_t length) : _data(data), _size(length) {
        }

        constexpr ArrayView(const T *begin, const T *end) : _data(begin), _size(end - begin) {
            assert(begin <= end);
        }

        template <template <class, class...> class V, class... A>
        ArrayView(const V<T, A...> &vec) : _data(vec.data()), _size(vec.size()) {
        }

        template <size_t N>
        constexpr ArrayView(const std::array<T, N> &Arr) : _data(Arr.data()), _size(N) {
        }

        template <size_t N>
        constexpr ArrayView(const T (&Arr)[N]) : _data(Arr), _size(N) {
        }

#if defined(__GNUC__) && __GNUC__ >= 9
// Disable gcc's warning in this constructor as it generates an enormous amount
// of messages. Anyone using ArrayRef(ArrayView) should already be aware of the fact that
// it does not do lifetime extension.
#  pragma GCC diagnostic push
#  pragma GCC diagnostic ignored "-Winit-list-lifetime"
#endif
        constexpr ArrayView(std::initializer_list<T> vec)
            : _data(vec.begin() == vec.end() ? (T *) nullptr : vec.begin()), _size(vec.size()) {
        }
#if defined(__GNUC__) && __GNUC__ >= 9
#  pragma GCC diagnostic pop
#endif

        template <typename T1>
        ArrayView(const ArrayView<T1 *> &RHS,
                   std::enable_if_t<std::is_convertible<T1 *const *, T const *>::value> * = nullptr)
            : _data(RHS.data()), _size(RHS.size()) {
        }

    public:
        iterator begin() const {
            return _data;
        }
        iterator end() const {
            return _data + _size;
        }
        reverse_iterator rbegin() const {
            return reverse_iterator(end());
        }
        reverse_iterator rend() const {
            return reverse_iterator(begin());
        }
        bool empty() const {
            return _size == 0;
        }
        const T *data() const {
            return _data;
        }
        size_t size() const {
            return _size;
        }

    public:
        const T &front() const {
            assert(!empty());
            return _data[0];
        }
        const T &back() const {
            assert(!empty());
            return _data[_size - 1];
        }
        bool equals(const ArrayView &RHS) const {
            if (_size != RHS._size)
                return false;
            return std::equal(begin(), end(), RHS.begin());
        }

        /// slice(i, j) - Chop off the first \p i elements of the array, and keep \p j
        /// elements in the array.
        ArrayView<T> slice(size_t i, size_t j) const {
            assert(i + j <= size() && "Invalid specifier");
            return ArrayView<T>(data() + i, j);
        }

        /// slice(n) - Chop off the first i elements of the array.
        ArrayView<T> slice(size_t i) const {
            return drop_front(i);
        }

        /// Drop the first \p i elements of the array.
        ArrayView<T> drop_front(size_t i = 1) const {
            assert(size() >= i && "Dropping more elements than exist");
            return slice(i, size() - i);
        }

        /// Drop the last \p i elements of the array.
        ArrayView<T> drop_back(size_t i = 1) const {
            assert(size() >= i && "Dropping more elements than exist");
            return slice(0, size() - i);
        }

        /// Return a copy of *this with only the first \p i elements.
        ArrayView<T> take_front(size_t i = 1) const {
            if (i >= size())
                return *this;
            return drop_back(size() - i);
        }

        /// Return a copy of *this with only the last \p i elements.
        ArrayView<T> take_back(size_t i = 1) const {
            if (i >= size())
                return *this;
            return drop_front(size() - i);
        }

        /// @}
        /// @name Operator Overloads
        /// @{
        const T &operator[](size_t index) const {
            assert(index < _size && "Invalid index!");
            return _data[index];
        }

        /// Disallow accidental assignment from a temporary.
        ///
        /// The declaration here is extra complicated so that "arrayRef = {}"
        /// continues to select the move assignment operator.
        template <typename T1>
        std::enable_if_t<std::is_same<T1, T>::value, ArrayView<T>> &
            operator=(T1 &&Temporary) = delete;

        /// Disallow accidental assignment from a temporary.
        ///
        /// The declaration here is extra complicated so that "arrayRef = {}"
        /// continues to select the move assignment operator.
        template <typename T1>
        std::enable_if_t<std::is_same<T1, T>::value, ArrayView<T>> &
            operator=(std::initializer_list<T1>) = delete;

        std::vector<T> vec() const {
            return std::vector<T>(_data, _data + _size);
        }

    private:
        const T *_data = nullptr;
        size_type _size = 0;
    };


    template <typename T>
    inline bool operator==(ArrayView<T> LHS, ArrayView<T> RHS) {
        return LHS.equals(RHS);
    }

    template <template <class, class...> class V, typename T, class... A>
    inline bool operator==(const V<T, A...> &LHS, ArrayView<T> RHS) {
        return ArrayView<T>(LHS).equals(RHS);
    }

    template <typename T>
    inline bool operator!=(ArrayView<T> LHS, ArrayView<T> RHS) {
        return !(LHS == RHS);
    }

    template <template <class, class...> class V, typename T, class... A>
    inline bool operator!=(const V<T, A...> &LHS, ArrayView<T> RHS) {
        return !(LHS == RHS);
    }

}

#endif // SUBSTATE_ARRAYVIEW_H