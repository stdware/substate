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

#endif // QSUBSTATE_GLOBAL_H
