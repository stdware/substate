#ifndef MAPPINGNODE_H
#define MAPPINGNODE_H

#include <unordered_map>
#include <vector>

#include <substate/node.h>
#include <substate/variant.h>

namespace Substate {

    class MappingNodePrivate;

    class SUBSTATE_EXPORT MappingNode : public Node {
        SUBSTATE_DECL_PRIVATE(MappingNode)
    public:
        MappingNode();
        ~MappingNode();

    public:
        class Value {
        public:
            inline Value();

            inline bool isValid() const;
            inline bool isVariant() const;
            inline bool isNode() const;

            inline Variant variant() const;
            inline Node *node() const;

        private:
            void *data;
            bool is_variant;

            inline Value(Node *node);
            inline Value(Variant *variant);

            friend class MappingNode;
            friend class MappingNodePrivate;
        };

        Value property(const std::string &key) const;
        bool setProperty(const std::string &key, const Variant &value);
        bool setProperty(const std::string &key, Node *node);
        inline bool clearProperty(const std::string &key);
        std::vector<std::string> keys() const;
        inline int count() const;
        int size() const;

    public:
        void write(OStream &stream) const override;
        Node *clone() const override;

    protected:
        void childDestroyed(Node *node) override;

    protected:
        MappingNode(MappingNodePrivate &d);
    };

    MappingNode::Value::Value() : data(nullptr), is_variant(false) {
    }

    bool MappingNode::Value::isValid() const {
        return data != nullptr;
    }

    bool MappingNode::Value::isVariant() const {
        return is_variant;
    }

    bool MappingNode::Value::isNode() const {
        return data && !is_variant;
    }

    Variant MappingNode::Value::variant() const {
        return is_variant ? *reinterpret_cast<Variant *>(data) : Variant();
    }

    Node *MappingNode::Value::node() const {
        return is_variant ? nullptr : reinterpret_cast<Node *>(data);
    }

    MappingNode::Value::Value(Node *node) : data(node), is_variant(true) {
    }

    MappingNode::Value::Value(Variant *variant) : data(variant), is_variant(false) {
    }

    bool MappingNode::clearProperty(const std::string &key) {
        return setProperty(key, Variant());
    }

    int MappingNode::count() const {
        return size();
    }

}

#endif // MAPPINGNODE_H
