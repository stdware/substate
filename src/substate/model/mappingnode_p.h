#ifndef MAPPINGNODE_P_H
#define MAPPINGNODE_P_H

#include <substate/mappingnode.h>
#include <substate/private/node_p.h>

namespace Substate {

    class SUBSTATE_EXPORT MappingNodePrivate : public NodePrivate {
        SUBSTATE_DECL_PUBLIC(MappingNode)
    public:
        MappingNodePrivate(const std::string &name);
        ~MappingNodePrivate();
        void init();

        static MappingNode *read(IStream &stream);

        std::unordered_map<std::string, Property> mapping;
        std::unordered_map<Node *, std::string> mappingIndexes;

        void setProperty_helper(const std::string &key, const Property &prop);
    };

    MappingAction *readMappingAction(IStream &stream);

}

#endif // MAPPINGNODE_P_H
