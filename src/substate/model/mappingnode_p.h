#ifndef MAPPINGNODE_P_H
#define MAPPINGNODE_P_H

#include <substate/mappingnode.h>
#include <substate/private/node_p.h>

namespace Substate {

    class SUBSTATE_EXPORT MappingNodePrivate : public NodePrivate {
        SUBSTATE_DECL_PUBLIC(MappingNode)
    public:
        MappingNodePrivate(int type);
        ~MappingNodePrivate();

        void init();

        static MappingNode *read(IStream &stream);

        struct Data {
            Variant variant;
            Node *node;
            bool is_variant;
        };
        std::unordered_map<std::string, Data> mapping;
    };

}

#endif // MAPPINGNODE_P_H
