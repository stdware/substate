// Copyright (C) 2022-2025 Stdware Collections (https://www.github.com/stdware)
// SPDX-License-Identifier: Apache-2.0

#ifndef SUBSTATE_QSUBSTATE_GLOBAL_H
#define SUBSTATE_QSUBSTATE_GLOBAL_H

#include <substate/substate_global.h>

#ifdef QSUBSTATE_STATIC
#  define QSUBSTATE_EXPORT
#else
#  ifdef QSUBSTATE_LIBRARY
#    define QSUBSTATE_EXPORT SUBSTATE_DECL_EXPORT
#  else
#    define QSUBSTATE_EXPORT SUBSTATE_DECL_IMPORT
#  endif
#endif

#endif // SUBSTATE_QSUBSTATE_GLOBAL_H