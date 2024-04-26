#ifndef SHEETNODE_H
#define SHEETNODE_H

#include <unordered_map>
#include <vector>

#include <substate/node.h>

namespace Substate {

    class SheetAction;

    class SheetNodePrivate;

    class SUBSTATE_EXPORT SheetNode : public Node {
        QMSETUP_DECL_PRIVATE(SheetNode)
    public:
        SheetNode(const std::string &name);
        ~SheetNode();

    public:
        int insert(Node *node);
        bool remove(int id);
        bool remove(Node *node);
        Node *record(int id) const;
        int indexOf(Node *node) const;
        std::vector<int> ids() const;
        const std::unordered_map<int, Node *> &data() const;
        inline int count() const;
        int size() const;

    public:
        void write(OStream &stream) const override;

    protected:
        Node *clone(bool user) const override;

        void childDestroyed(Node *node) override;
        void propagateChildren(const std::function<void(Node *)> &func) override;

    protected:
        SheetNode(SheetNodePrivate &d);

        friend class SheetAction;
    };

    inline int SheetNode::count() const {
        return size();
    }

    class SUBSTATE_EXPORT SheetAction : public NodeAction {
    public:
        SheetAction(Type type, Node *parent, int id, Node *child);
        ~SheetAction();

    public:
        void write(OStream &stream) const override;
        Action *clone() const override;
        void execute(bool undo) override;
        void virtual_hook(int id, void *data) override;

    public:
        inline int id() const;
        inline Node *child() const;

    protected:
        int m_id;
        Node *m_child;
    };

    inline int SheetAction::id() const {
        return m_id;
    }

    inline Node *SheetAction::child() const {
        return m_child;
    }

}

#endif // SHEETNODE_H
