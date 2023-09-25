#ifndef SUBSTATE_GLOBAL_H
#define SUBSTATE_GLOBAL_H

// Export define
#ifdef _WIN32
#  define SUBSTATE_DECL_EXPORT __declspec(dllexport)
#  define SUBSTATE_DECL_IMPORT __declspec(dllimport)
#else
#  define SUBSTATE_DECL_EXPORT
#  define SUBSTATE_DECL_IMPORT
#endif

#ifdef SUBSTATE_STATIC
#  define SUBSTATE_EXPORT
#else
#  ifdef SUBSTATE_LIBRARY
#    define SUBSTATE_EXPORT SUBSTATE_DECL_EXPORT
#  else
#    define SUBSTATE_EXPORT SUBSTATE_DECL_IMPORT
#  endif
#endif

// Qt style P-IMPL
#define SUBSTATE_DECL_PRIVATE(Class)                                                               \
    inline Class##Private *d_func() {                                                              \
        return reinterpret_cast<Class##Private *>(d_ptr.get());                                    \
    }                                                                                              \
    inline const Class##Private *d_func() const {                                                  \
        return reinterpret_cast<const Class##Private *>(d_ptr.get());                              \
    }                                                                                              \
    friend class Class##Private;

#define SUBSTATE_DECL_PUBLIC(Class)                                                                \
    inline Class *q_func() {                                                                       \
        return static_cast<Class *>(q_ptr);                                                        \
    }                                                                                              \
    inline const Class *q_func() const {                                                           \
        return static_cast<const Class *>(q_ptr);                                                  \
    }                                                                                              \
    friend class Class;

#ifndef Q_D
#  define Q_D(Class) Class##Private *const d = d_func()
#endif

#ifndef Q_Q
#  define Q_Q(Class) Class *const q = q_func()
#endif

// Some classes do not permit copies to be made of an object.
#define SUBSTATE_DISABLE_COPY(Class)                                                               \
    Class(const Class &) = delete;                                                                 \
    Class &operator=(const Class &) = delete;

#define SUBSTATE_DISABLE_MOVE(Class)                                                               \
    Class(Class &&) = delete;                                                                      \
    Class &operator=(Class &&) = delete;

#define SUBSTATE_DISABLE_COPY_MOVE(Class)                                                          \
    SUBSTATE_DISABLE_COPY(Class)                                                                   \
    SUBSTATE_DISABLE_MOVE(Class)

#endif // SUBSTATE_GLOBAL_H
