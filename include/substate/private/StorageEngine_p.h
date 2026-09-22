// Copyright (C) 2022-2025 Stdware Collections (https://www.github.com/stdware)
// SPDX-License-Identifier: Apache-2.0

#ifndef SUBSTATE_STORAGEENGINE_P_H
#define SUBSTATE_STORAGEENGINE_P_H

#include <substate/StorageEngine.h>

#include <substate/private/Node_p.h>

namespace ss {

    class SUBSTATE_EXPORT StorageEnginePrivate {
    public:
        static inline size_t addNode(StorageEngine *self, NodePtr node, size_t id = 0) {
            size_t newId = id > 0 ? (self->_maxId = std::max(self->_maxId, id), id) : (++self->_maxId);
            self->_nodeMap[newId] = std::move(node);
            NodePrivate::setId(node.get(), newId);
            return newId;
        }

        static inline void removeNode(StorageEngine *self, size_t id) {
            self->_nodeMap.erase(id);
        }
    };

}

#endif // SUBSTATE_STORAGEENGINE_P_H
