#ifndef SUBSTATE_TWOPHASEOBJECT_H
#define SUBSTATE_TWOPHASEOBJECT_H

#include <type_traits>
#include <cassert>

namespace ss {

    /// TwoPhaseObject - Stores an object that have a temporary state but does NOT track whether
    /// they are in this state.
    /// \tparam T - The type of the normal payload.
    /// \tparam U - The type of the temporary payload.
    ///
    /// \warning After constructing a \c TwoPhaseObject with the temporary payload, call \c load()
    /// in time to transition it to the normal state. Failure to do so may result in memory leaks.
    template <class T, class U>
    struct TwoPhaseObject {
        using value_type = T;
        using temp_type = U;

        /// Construct with a temporary payload.
        TwoPhaseObject(const temp_type &temp)
#ifndef NDEBUG
            : isTemporary(true)
#endif
        {
            new (&storage.temp) temp_type(temp);
        }
        TwoPhaseObject(temp_type &&temp)
#ifndef NDEBUG
            : isTemporary(true)
#endif
        {
            new (&storage.temp) temp_type(std::move(temp));
        }

        /// Construct with a normal payload.
        TwoPhaseObject(const value_type &value)
#ifndef NDEBUG
            : isTemporary(false)
#endif
        {
            new (&storage.value) value_type(value);
        }
        TwoPhaseObject(value_type &&value)
#ifndef NDEBUG
            : isTemporary(false)
#endif
        {
            new (&storage.value) value_type(std::move(value));
        }

        /// Copy/move constructor, only available when the object is in the normal state.
        TwoPhaseObject(const TwoPhaseObject &RHS)
#ifndef NDEBUG
            : isTemporary(false)
#endif
        {
#ifndef NDEBUG
            assert(!RHS.isTemporary);
#endif
            new (&storage.value) value_type(RHS.value());
        }
        TwoPhaseObject(TwoPhaseObject &&RHS)
#ifndef NDEBUG
            : isTemporary(false)
#endif
        {
#ifndef NDEBUG
            assert(!RHS.isTemporary);
#endif
            new (&storage.value) value_type(std::move(RHS.value()));
        }

        /// Destructor.
        /// \note The destructor only destroys the normal payload, so it must be called only when
        /// the object is in the normal state, otherwise it will cause undefined behavior.
        ~TwoPhaseObject() {
#ifndef NDEBUG
            assert(!isTemporary);
#endif
            destructValue();
        }

        /// Assign operators, only available when the object is in the normal state.
        TwoPhaseObject &operator=(const TwoPhaseObject &RHS) {
#ifndef NDEBUG
            assert(!isTemporary && !RHS.isTemporary);
#endif
            if (this != &RHS) {
                value() = RHS.value();
            }
            return *this;
        }
        TwoPhaseObject &operator=(TwoPhaseObject &&RHS) {
#ifndef NDEBUG
            assert(!isTemporary && !RHS.isTemporary);
#endif
            if (this != &RHS) {
                value() = std::move(RHS.value());
            }
            return *this;
        }

        /// Transition the object to the normal state.
        /// \note This function destroys the temporary payload, so it must be called only when
        /// the object is in the temporary state, otherwise it will cause undefined behavior.
        void load(const value_type &value) {
#ifndef NDEBUG
            assert(isTemporary);
            isTemporary = false;
#endif
            destructTemp();
            new (&storage.value) value_type(value);
        }
        void load(value_type &&value) {
#ifndef NDEBUG
            assert(isTemporary);
            isTemporary = false;
#endif
            destructTemp();
            new (&storage.value) value_type(std::move(value));
        }

        /// Transition the object to the temporary state.
        /// \note This function destroys the normal payload, so it must be called only when
        /// the object is in the normal state, otherwise it will cause undefined behavior.
        void unload(const temp_type &temp) {
#ifndef NDEBUG
            assert(!isTemporary);
            isTemporary = true;
#endif
            destructValue();
            new (&storage.temp) temp_type(temp);
        }
        void unload(temp_type &&temp) {
#ifndef NDEBUG
            assert(!isTemporary);
            isTemporary = true;
#endif
            destructValue();
            new (&storage.temp) temp_type(std::move(temp));
        }

        /// Access the temporary payload, only available when the object is in the temporary state.
        temp_type &temp() {
#ifndef NDEBUG
            assert(isTemporary);
#endif
            return storage.temp;
        }
        const temp_type &temp() const {
#ifndef NDEBUG
            assert(isTemporary);
#endif
            return storage.temp;
        }

        /// Access the normal payload, only available when the object is in the normal state.
        value_type &value() {
#ifndef NDEBUG
            assert(!isTemporary);
#endif
            return storage.value;
        }
        const value_type &value() const {
#ifndef NDEBUG
            assert(!isTemporary);
#endif
            return storage.value;
        }
        operator value_type &() {
#ifndef NDEBUG
            assert(!isTemporary);
#endif
            return storage.value;
        }
        operator const value_type &() const {
#ifndef NDEBUG
            assert(!isTemporary);
#endif
            return storage.value;
        }
        value_type *operator->() {
#ifndef NDEBUG
            assert(!isTemporary);
#endif
            return &storage.value;
        }
        const value_type *operator->() const {
#ifndef NDEBUG
            assert(!isTemporary);
#endif
            return &storage.value;
        }

        /// Assign normal payload, only available when the object is in the normal state.
        TwoPhaseObject &operator=(const value_type &RHS) {
#ifndef NDEBUG
            assert(!isTemporary);
#endif
            value() = RHS;
            return *this;
        }
        TwoPhaseObject &operator=(value_type &&RHS) {
#ifndef NDEBUG
            assert(!isTemporary);
#endif
            value() = std::move(RHS);
            return *this;
        }

        /// Assign temporary payload, only available when the object is in the normal state.
        /// \note Transition the object to the temporary state.
        TwoPhaseObject &operator=(const temp_type &RHS) {
            unload(RHS);
            return *this;
        }
        TwoPhaseObject &operator=(temp_type &&RHS) {
            unload(std::move(RHS));
            return *this;
        }

    private:
        union Storage {
            temp_type temp;
            value_type value;

            Storage(){};
            ~Storage(){};
        };
        Storage storage;
#ifndef NDEBUG
        bool isTemporary;
#endif

        void destructTemp() {
            if constexpr (!std::is_trivially_destructible_v<temp_type>)
                storage.temp.~temp_type();
        }

        void destructValue() {
            if constexpr (!std::is_trivially_destructible_v<value_type>)
                storage.value.~value_type();
        }
    };

}

#endif // SUBSTATE_TWOPHASEOBJECT_H