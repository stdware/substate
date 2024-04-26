#ifndef STRUCTNODE_P_H
#define STRUCTNODE_P_H

#include <substate/structnode.h>
#include <substate/private/node_p.h>

namespace Substate {

    class SUBSTATE_EXPORT StructNodePrivate : public NodePrivate {
        QMSETUP_DECL_PUBLIC(StructNode)
    public:
        StructNodePrivate(const std::string &name, int size);
        ~StructNodePrivate();
        void init();

        static StructNode *read(IStream &stream);

        std::vector<Property> array;
        std::unordered_map<Node *, int> arrayIndexes;

        void assign_helper(int i, const Property &prop);
    };

    StructAction *readStructAction(IStream &stream);

}

#endif // STRUCTNODE_P_H
