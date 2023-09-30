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

            Data(const Variant &var) : variant(var), node(nullptr), is_variant(true){};
            Data(Node *node) : node(node), is_variant(false){};
        };
        std::unordered_map<std::string, Data> mapping;
        std::unordered_map<Node *, std::string> mappingIndexes;

        void setProperty_helper(const std::string &key, const Data &data);


        static inline MappingNode::Value createValue(Node *node) {
            return node;
        }

        static inline MappingNode::Value createValue(Variant *var) {
            return var;
        }
    };

}

#endif // MAPPINGNODE_P_H
