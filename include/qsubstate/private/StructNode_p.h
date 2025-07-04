
// Copyright (C) 2022-2025 Stdware Collections (https://www.github.com/stdware)
// SPDX-License-Identifier: Apache-2.0

#ifndef SUBSTATE_STRUCTNODE_P_H
#define SUBSTATE_STRUCTNODE_P_H

#include <optional>

#include <qsubstate/StructNode.h>

namespace ss {

    class QSUBSTATE_EXPORT StructNodeBasePrivate {
    public:
        static void copy(StructNodeBase *dest, const StructNodeBase *src, bool copyId);
    };

}

#endif // SUBSTATE_STRUCTNODE_P_H
