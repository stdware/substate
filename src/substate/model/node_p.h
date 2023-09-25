#ifndef NODE_P_H
#define NODE_P_H

#include <substate/node.h>

namespace Substate {

    class SUBSTATE_EXPORT NodePrivate {
        SUBSTATE_DECL_PUBLIC(Node)
    public:
        NodePrivate(int type);
        virtual ~NodePrivate();

        void init();

        Node *q_ptr;

        int type;
        Node *parent;
        Model *model;
    };

}

#endif // NODE_P_H
