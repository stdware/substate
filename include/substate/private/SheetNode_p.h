// Copyright (C) 2022-2025 Stdware Collections (https://www.github.com/stdware)
// SPDX-License-Identifier: Apache-2.0

#ifndef SUBSTATE_SHEETNODE_P_H
#define SUBSTATE_SHEETNODE_P_H

#include <substate/SheetNode.h>

namespace ss {

    class SUBSTATE_EXPORT SheetNodePrivate {
    public:
        static void insert_TX(SheetNode *q, int id, const std::shared_ptr<Node> &node);

        static bool remove_TX(SheetNode *q, int id);

        static void copy(SheetNode *dest, const SheetNode *src, bool copyId);
    };

}

#endif // SUBSTATE_SHEETNODE_P_H