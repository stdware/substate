#ifndef SUBSTATE_GLOBAL_H
#define SUBSTATE_GLOBAL_H

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



#endif // SUBSTATE_GLOBAL_H
