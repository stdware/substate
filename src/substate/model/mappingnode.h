#ifndef MAPPINGNODE_H
#define MAPPINGNODE_H

#include <unordered_map>
#include <vector>

#include <substate/property.h>

namespace Substate {

    class MappingAction;

    class MappingNodePrivate;

    class SUBSTATE_EXPORT MappingNode : public Node {
        QMSETUP_DECL_PRIVATE(MappingNode)
    public:
        MappingNode();
        ~MappingNode();

    public:
        Property property(const std::string &key) const;
        void setProperty(const std::string &key, const Property &value);
        inline void clearProperty(const std::string &key);
        bool remove(Node *node);
        std::string indexOf(Node *node) const;
        std::vector<std::string> keys() const;
        const std::unordered_map<std::string, Property> &data() const;
        inline int count() const;
        int size() const;

        inline void insert(const std::string &key, const Property &value);
        inline void remove(const std::string &key);
        inline Property at(const std::string &key);

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

    inline void MappingNode::clearProperty(const std::string &key) {
        return setProperty(key, {});
    }

    inline int MappingNode::count() const {
        return size();
    }

    inline void MappingNode::insert(const std::string &key, const Property &value) {
        setProperty(key, value);
    }

    inline void MappingNode::remove(const std::string &key) {
        clearProperty(key);
    }

    inline Property MappingNode::at(const std::string &key) {
        return property(key);
    }

    class SUBSTATE_EXPORT MappingAction : public PropertyAction {
    public:
        MappingAction(Node *parent, const std::string &key, const Property &value,
                      const Property &oldValue);
        ~MappingAction();

    public:
        void write(OStream &stream) const override;
        Action *clone() const override;
        void execute(bool undo) override;

    public:
        inline std::string key() const;

    public:
        std::string m_key;
    };

    inline std::string MappingAction::key() const {
        return m_key;
    }

}

#endif // MAPPINGNODE_H
