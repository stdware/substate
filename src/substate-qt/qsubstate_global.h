#ifndef QSUBSTATE_GLOBAL_H
#define QSUBSTATE_GLOBAL_H

#include <QtCore/QtGlobal>

#ifdef QSUBQSTATE_STATIC
#  define QSUBSTATE_EXPORT
#else
#  ifdef QSUBSTATE_LIBRARY
#    define QSUBSTATE_EXPORT Q_DECL_EXPORT
#  else
#    define QSUBSTATE_EXPORT Q_DECL_IMPORT
#  endif
#endif

#if defined(Q_CC_MSVC)
#  define QSUBSTATE_NOINLINE __declspec(noinline)
#  define QSUBSTATE_INLINE   __forceinline
#  define QSUBSTATE_USED
#else
#  define QSUBSTATE_NOINLINE __attribute__((noinline))
#  define QSUBSTATE_INLINE   __attribute__((always_inline))
#  define QSUBSTATE_USED     __attribute__((used))
#endif

#endif // QSUBSTATE_GLOBAL_H
