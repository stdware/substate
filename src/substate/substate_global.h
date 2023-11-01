#ifndef SUBSTATE_GLOBAL_H
#define SUBSTATE_GLOBAL_H

#include <qtmediateCM/qtmediate_global.h>

#ifdef SUBSTATE_STATIC
#  define SUBSTATE_EXPORT
#else
#  ifdef SUBSTATE_LIBRARY
#    define SUBSTATE_EXPORT QTMEDIATE_DECL_EXPORT
#  else
#    define SUBSTATE_EXPORT QTMEDIATE_DECL_IMPORT
#  endif
#endif

#endif // SUBSTATE_GLOBAL_H
