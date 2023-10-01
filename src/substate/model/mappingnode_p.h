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

        std::unordered_map<std::string, Property> mapping;
        std::unordered_map<Node *, std::string> mappingIndexes;

        void setProperty_helper(const std::string &key, const Property &prop);
    };

    Action *readMappingAction(IStream &stream,
                              const std::unordered_map<int, Node *> &existingNodes);

}

#endif // MAPPINGNODE_P_H
