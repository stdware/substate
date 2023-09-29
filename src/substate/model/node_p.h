#ifndef NODE_P_H
#define NODE_P_H

#include <substate/node.h>
#include <substate/private/sender_p.h>

namespace Substate {

    class SUBSTATE_EXPORT NodePrivate : public SenderPrivate {
        SUBSTATE_DECL_PUBLIC(Node)
    public:
        NodePrivate(int type);
        ~NodePrivate();
        void init();

        int type;
        Node *parent;
        Model *model;
        int index;
        bool managed;
        bool allowDelete;

        void setManaged(bool _managed);
        void propagateModel(Model *_model);

        bool testModifiable() const;
        inline bool testInsertable(const Node *item) const {
            return item && item->isFree();
        }
    };

}

#endif // NODE_P_H
