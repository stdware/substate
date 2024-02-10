#ifndef NODEHELPER_H
#define NODEHELPER_H

#include <substate/node.h>
#include <substate/engine.h>

namespace Substate {

    class NodeExtra;

    class SUBSTATE_EXPORT NodeHelper {
    public:
        static inline NodePrivate *get(Node *node);
        static inline Node *clone(Node *node, bool user);
        static inline void propagateNode(Node *node, const std::function<void(Node *)> &func);

        static void setIndex(Node *node, int index);
        static void setManaged(Node *node, bool managed);
        static NodeExtra *getExtra(Node *node);
        static void setExtra(Node *node, NodeExtra *extra);

        static void setModelRoot(Node *node, Model *model);
        static void propagateEngine(Node *node, Engine *engine);

        static void forceDelete(Node *node);
    };

    inline NodePrivate *NodeHelper::get(Node *node) {
        return node->d_func();
    }

    inline Node *NodeHelper::clone(Substate::Node *node, bool user) {
        return node->clone(user);
    }

    inline void NodeHelper::propagateNode(Node *node, const std::function<void(Node *)> &func) {
        node->propagate(func);
    }

}



#endif // NODEHELPER_H
