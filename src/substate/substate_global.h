#ifndef SUBSTATE_GLOBAL_H
#define SUBSTATE_GLOBAL_H

#include <qmsetup/qmsetup_global.h>

#ifdef SUBSTATE_STATIC
#  define SUBSTATE_EXPORT
#else
#  ifdef SUBSTATE_LIBRARY
#    define SUBSTATE_EXPORT QMSETUP_DECL_EXPORT
#  else
#    define SUBSTATE_EXPORT QMSETUP_DECL_IMPORT
#  endif
#endif

#endif // SUBSTATE_GLOBAL_H
