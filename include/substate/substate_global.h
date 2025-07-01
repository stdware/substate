// Copyright (C) 2022-2025 Stdware Collections (https://www.github.com/stdware)
// SPDX-License-Identifier: Apache-2.0

#ifndef SUBSTATE_SUBSTATE_GLOBAL_H
#define SUBSTATE_SUBSTATE_GLOBAL_H

#ifdef _WIN32
#  define SUBSTATE_DECL_EXPORT __declspec(dllexport)
#  define SUBSTATE_DECL_IMPORT __declspec(dllimport)
#else
#  define SUBSTATE_DECL_EXPORT __attribute__((visibility("default")))
#  define SUBSTATE_DECL_IMPORT __attribute__((visibility("default")))
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

#endif // SUBSTATE_SUBSTATE_GLOBAL_H