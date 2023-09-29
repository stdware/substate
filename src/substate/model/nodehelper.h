#ifndef NODEHELPER_H
#define NODEHELPER_H

#include <substate/node.h>

namespace Substate {

    class SUBSTATE_EXPORT NodeHelper {
    public:
        static inline NodePrivate *get(Node *node);

        static inline Node *clone(Node *node, bool user);

        static void setIndex(Node *node, int index);
        static void setManaged(Node *node, bool managed);

        static void propagateModel(Node *node, Model *model);
        static void forceDelete(Node *node);
    };

    inline NodePrivate *NodeHelper::get(Node *node) {
        return node->d_func();
    }

    inline Node *NodeHelper::clone(Substate::Node *node, bool user) {
        return node->clone(user);
    }

}



#endif // NODEHELPER_H
