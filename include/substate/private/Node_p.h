// Copyright (C) 2022-2025 Stdware Collections (https://www.github.com/stdware)
// SPDX-License-Identifier: Apache-2.0

#ifndef SUBSTATE_NODE_P_H
#define SUBSTATE_NODE_P_H

#include <substate/Node.h>

namespace ss {

    class SUBSTATE_EXPORT NodePrivate {
    public:
        /// Call \c Node 's protected \c propagate method.
        static inline void propagate(NodePtr &node, const std::function<void(NodePtr &)> &func) {
            func(node);
            node->propagateChildren(func);
        }

        /// Associates the node and all its descendants with a model.
        static void propagate(NodePtr &node, Model *model);

        /// Sets the id of the node silently.
        static inline void setId(Node *node, size_t id) {
            node->_id = id;
        }

        static inline void setModel(Node *node, Model *model) {
            node->_model = model;
        }

        // Debug use
        static inline bool validateArrayQueryArguments(int index, int size) {
            return index >= 0 && index <= size;
        }

        // Debug use
        static inline bool validateArrayRemoveArguments(int index, int count, int size) {
            return (index >= 0 && index < size)            // index bound
                   && (count > 0 && count <= size - index) // count bound
                ;
        }
    };

}

#endif // SUBSTATE_NODE_P_H
