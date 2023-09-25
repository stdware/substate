#ifndef VECTORNODE_P_H
#define VECTORNODE_P_H

#include <substate/vectornode.h>
#include <substate/private/node_p.h>

namespace Substate {

    class SUBSTATE_EXPORT VectorNodePrivate : public NodePrivate {
        SUBSTATE_DECL_PUBLIC(VectorNode)
    public:
        VectorNodePrivate(int type);
        ~VectorNodePrivate();

        void init();

        static VectorNode *read(IStream &stream);

        std::vector<Node *> vector;
    };

}

#endif // VECTORNODE_P_H
