#ifndef MAPPINGNODE_H
#define MAPPINGNODE_H

#include <unordered_map>
#include <vector>

#include <substate/node.h>
#include <substate/variant.h>

namespace Substate {

    class MappingAction;

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

            inline void *internalPointer() const;

        public:
            inline bool operator==(const Value &other) const;

        private:
            void *data;
            bool is_variant;

            inline Value(Node *node);
            inline Value(Variant *variant);

            friend class MappingNode;
            friend class MappingNodePrivate;
        };

        Value property(const std::string &key) const;
        void setProperty(const std::string &key, const Variant &value);
        void setProperty(const std::string &key, Node *node);
        inline void clearProperty(const std::string &key);
        std::vector<std::string> keys() const;
        inline int count() const;
        int size() const;

    public:
        void write(OStream &stream) const override;

    protected:
        Node *clone(bool user) const override;

        void childDestroyed(Node *node) override;
        void propagateChildren(const std::function<void(Node *)> &func) override;

    protected:
        MappingNode(MappingNodePrivate &d);

        friend class MappingAction;
    };

    MappingNode::Value::Value() : data(nullptr), is_variant(false) {
    }

    inline bool MappingNode::Value::isValid() const {
        return data != nullptr;
    }

    inline bool MappingNode::Value::isVariant() const {
        return is_variant;
    }

    inline bool MappingNode::Value::isNode() const {
        return data && !is_variant;
    }

    inline Variant MappingNode::Value::variant() const {
        return is_variant ? *reinterpret_cast<Variant *>(data) : Variant();
    }

    inline Node *MappingNode::Value::node() const {
        return is_variant ? nullptr : reinterpret_cast<Node *>(data);
    }

    inline void *MappingNode::Value::internalPointer() const {
        return data;
    }

    bool MappingNode::Value::operator==(const MappingNode::Value &other) const {
        if (is_variant) {
            return other.is_variant && variant() == other.variant();
        }
        return node() == other.node();
    }

    inline MappingNode::Value::Value(Node *node) : data(node), is_variant(true) {
    }

    inline MappingNode::Value::Value(Variant *variant) : data(variant), is_variant(false) {
    }

    inline void MappingNode::clearProperty(const std::string &key) {
        return setProperty(key, Variant());
    }

    inline int MappingNode::count() const {
        return size();
    }

    class SUBSTATE_EXPORT MappingAction : public NodeAction {
    public:
        MappingAction(Node *parent, const std::string &key, const MappingNode::Value &value,
                      const MappingNode::Value &oldValue);
        ~MappingAction();

    public:
        void write(OStream &stream) const override;
        Action *clone() const override;
        void execute(bool undo) override;
        void virtual_hook(int id, void *data) override;

    public:
        inline std::string key() const;
        inline MappingNode::Value value() const;
        inline MappingNode::Value oldValue() const;

    public:
        std::string m_key;
        MappingNode::Value v, oldv;
    };

    inline std::string MappingAction::key() const {
        return m_key;
    }

    inline MappingNode::Value MappingAction::value() const {
        return v;
    }

    inline MappingNode::Value MappingAction::oldValue() const {
        return oldv;
    }

}

#endif // MAPPINGNODE_H
