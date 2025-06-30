#ifndef SUBSTATE_NODE_P_H
#define SUBSTATE_NODE_P_H

#include <substate/Node.h>

namespace ss {

    class SUBSTATE_EXPORT NodePrivate {
    public:
        /// Call \c Node 's protected \c clone method.
        static inline std::shared_ptr<Node> clone(Node *node, bool copyId) {
            return node->clone(copyId);
        }

        /// Call \c Node 's protected \c propagate method.
        static inline void propagate(Node *node, const std::function<void(Node *)> &func) {
            node->propagate(func);
        }

        /// Associates the node and all its descendants with a model.
        static void propagate(Node *node, Model *model);

        /// Sets the id of the node silently.
        static inline void setId(Node *node, size_t id) {
            node->_id = id;
        }
    };

}

#endif // SUBSTATE_NODE_P_H