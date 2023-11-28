#ifndef VECTORNODE_P_H
#define VECTORNODE_P_H

#include <substate/vectornode.h>
#include <substate/private/node_p.h>

namespace Substate {

    class SUBSTATE_EXPORT VectorNodePrivate : public NodePrivate {
        QMSETUP_DECL_PUBLIC(VectorNode)
    public:
        VectorNodePrivate(int type);
        ~VectorNodePrivate();
        void init();

        static VectorNode *read(IStream &stream);

        void insertRows_helper(int index, const std::vector<Node *> &nodes);
        void moveRows_helper(int index, int count, int dest);
        void removeRows_helper(int index, int count);

        std::vector<Node *> vector;
    };

    VectorMoveAction *readVectorMoveAction(IStream &stream,
                                 const std::unordered_map<int, Node *> &existingNodes);

    VectorInsDelAction *readVectorInsDelAction(Action::Type type, IStream &stream,
                                   const std::unordered_map<int, Node *> &existingNodes);

}

#endif // VECTORNODE_P_H
