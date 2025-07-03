// Copyright (C) 2022-2025 Stdware Collections (https://www.github.com/stdware)
// SPDX-License-Identifier: Apache-2.0

#ifndef SUBSTATE_SHEETNODE_P_H
#define SUBSTATE_SHEETNODE_P_H

#include <optional>

#include <substate/SheetNode.h>

namespace ss {

    class SUBSTATE_EXPORT SheetNodePrivate {
    public:
        static void copy(SheetNode *dest, const SheetNode *src, bool copyId);
    };

}

#endif // SUBSTATE_SHEETNODE_P_H