#ifndef STRUCTNODE_H
#define STRUCTNODE_H

#include <substate/property.h>

namespace Substate {

    class StructAction;

    class StructNodePrivate;

    class SUBSTATE_EXPORT StructNode : public Node {
        QMSETUP_DECL_PRIVATE(StructNode)
    public:
        StructNode(int size);
        ~StructNode();

    protected:
        bool assign(int i, const Property &value);
        Property at(int i) const;
        bool remove(Node *node);
        int indexOf(Node *node) const;
        const std::vector<Property> &data() const;
        inline int count() const;
        int size() const;

    public:
        void write(OStream &stream) const override;

    protected:
        Node *clone(bool user) const override;

        void childDestroyed(Node *node) override;
        void propagateChildren(const std::function<void(Node *)> &func) override;

    protected:
        StructNode(StructNodePrivate &d);

        friend class StructAction;
    };

    inline int StructNode::count() const {
        return size();
    }

    class SUBSTATE_EXPORT StructAction : public PropertyAction {
    public:
        StructAction(Node *parent, int index, const Property &value, const Property &oldValue);
        ~StructAction();

    public:
        void write(OStream &stream) const override;
        Action *clone() const override;
        void execute(bool undo) override;

    public:
        inline int index() const;

    protected:
        int m_index;
    };

    inline int StructAction::index() const {
        return m_index;
    }

}



#endif // STRUCTNODE_H
