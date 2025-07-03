// Copyright (C) 2022-2025 Stdware Collections (https://www.github.com/stdware)
// SPDX-License-Identifier: Apache-2.0

#ifndef SUBSTATE_VECTORNODE_P_H
#define SUBSTATE_VECTORNODE_P_H

#include <substate/VectorNode.h>

namespace ss {

    class SUBSTATE_EXPORT VectorNodePrivate {
    public:
        static void copy(VectorNode *dest, const VectorNode *src, bool copyId);
    };

}

#endif // SUBSTATE_VECTORNODE_P_H