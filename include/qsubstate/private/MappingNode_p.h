// Copyright (C) 2022-2025 Stdware Collections (https://www.github.com/stdware)
// SPDX-License-Identifier: Apache-2.0

#ifndef SUBSTATE_MAPPINGNODE_P_H
#define SUBSTATE_MAPPINGNODE_P_H

#include <optional>

#include <qsubstate/MappingNode.h>

namespace ss {

    class QSUBSTATE_EXPORT MappingNodePrivate {
    public:
        static void copy(MappingNode *dest, const MappingNode *src, bool copyId);
    };

}

#endif // SUBSTATE_MAPPINGNODE_P_H
