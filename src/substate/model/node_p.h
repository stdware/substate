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

    inline bool validateArrayQueryArguments(int index, int size) {
        return index >= 0 && index <= size;
    }

    inline bool validateArrayRemoveArguments(int index, int &count, int size) {
        return (index >= 0 && index <= size)                                     // index bound
               && ((count = std::min(count, size - index)) > 0 && count <= size) // count bound
            ;
    }

    Action *readRootChangeAction(IStream &stream,
                                 const std::unordered_map<int, Node *> &existingNodes);

}

#endif // NODE_P_H
