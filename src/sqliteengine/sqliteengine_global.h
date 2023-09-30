#ifndef SQLITEENGINE_GLOBAL_H
#define SQLITEENGINE_GLOBAL_H

#include <substate/substate_global.h>

// Export define
#ifdef SQLITEENGINE_STATIC
#  define SQLITEENGINE_EXPORT
#else
#  ifdef SQLITEENGINE_LIBRARY
#    define SQLITEENGINE_EXPORT SUBSTATE_DECL_EXPORT
#  else
#    define SQLITEENGINE_EXPORT SUBSTATE_DECL_IMPORT
#  endif
#endif

#endif // SQLITEENGINE_GLOBAL_H
