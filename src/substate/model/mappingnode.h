#ifndef MAPPINGNODE_H
#define MAPPINGNODE_H

#include <unordered_map>
#include <vector>

#include <substate/property.h>

namespace Substate {

    class MappingAction;

    class MappingNodePrivate;

    class SUBSTATE_EXPORT MappingNode : public Node {
        SUBSTATE_DECL_PRIVATE(MappingNode)
    public:
        MappingNode();
        ~MappingNode();

    public:
        Property property(const std::string &key) const;
        void setProperty(const std::string &key, const Property &value);
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

    inline void MappingNode::clearProperty(const std::string &key) {
        return setProperty(key, {});
    }

    inline int MappingNode::count() const {
        return size();
    }

    class SUBSTATE_EXPORT MappingAction : public NodeAction {
    public:
        MappingAction(Node *parent, const std::string &key, const Property &value,
                      const Property &oldValue);
        ~MappingAction();

    public:
        void write(OStream &stream) const override;
        Action *clone() const override;
        void execute(bool undo) override;
        void virtual_hook(int id, void *data) override;

    public:
        inline std::string key() const;
        inline Property value() const;
        inline Property oldValue() const;

    public:
        std::string m_key;
        Property v, oldv;
    };

    inline std::string MappingAction::key() const {
        return m_key;
    }

    inline Property MappingAction::value() const {
        return v;
    }

    inline Property MappingAction::oldValue() const {
        return oldv;
    }

}

#endif // MAPPINGNODE_H
