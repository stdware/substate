// Copyright (C) 2022-2025 Stdware Collections (https://www.github.com/stdware)
// SPDX-License-Identifier: Apache-2.0

#ifndef SUBSTATE_BYTESNODE_P_H
#define SUBSTATE_BYTESNODE_P_H

#include <substate/BytesNode.h>

namespace ss {

    class SUBSTATE_EXPORT BytesNodePrivate {
    public:
        static void copy(BytesNode *dest, const BytesNode *src, bool copyId);
    };

}

#endif // SUBSTATE_BYTESNODE_P_H