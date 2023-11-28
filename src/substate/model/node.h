#ifndef NODE_H
#define NODE_H

#include <functional>
#include <memory>
#include <string>

#include <substate/action.h>
#include <substate/sender.h>
#include <substate/stream.h>

namespace Substate {

    class Model;

    class ModelPrivate;

    class NodeHelper;

    class NodeExtra;

    class NodePrivate;

    class SUBSTATE_EXPORT Node : public Sender {
        QMSETUP_DECL_PRIVATE(Node)
    public:
        enum Type {
            Bytes,
            Vector,
            Mapping,
            Sheet,
            Struct,
            User = 1024,
        };

        explicit Node(int type);
        ~Node();

        int type() const;

        typedef Node *(*Factory)(IStream &);

    public:
        Node *parent() const;
        Model *model() const;
        int index() const;

        bool isFree() const;
        inline bool isObsolete() const;
        bool isManaged() const;
        bool isWritable() const;

        virtual void write(OStream &stream) const = 0;
        Node *clone() const;

        static Node *read(IStream &stream);
        static bool registerFactory(int type, Factory fac);

    public:
        void dispatch(Notification *n) override;

    protected:
        virtual Node *clone(bool user) const = 0;

        virtual void childDestroyed(Node *node);
        virtual void propagateChildren(const std::function<void(Node *)> &func);

    protected:
        void addChild(Node *node);
        void removeChild(Node *node);

        void beginAction();
        void endAction();

        void propagate(const std::function<void(Node *)> &func);

    protected:
        Node(NodePrivate &d);

        friend class Model;
        friend class ModelPrivate;
        friend class NodeHelper;
        friend class NodeExtra;
    };

    inline bool Node::isObsolete() const {
        return isManaged();
    }

    class SUBSTATE_EXPORT NodeExtra {
    public:
        NodeExtra();
        NodeExtra(Node *node);
        virtual ~NodeExtra();

        inline Node *internalData() const;

    protected:
        void setInternalData(Node *node);

        virtual void notified(Notification *n) = 0;

    protected:
        Node *m_node;

        friend class Node;
        friend class NodePrivate;
    };

    inline Node *NodeExtra::internalData() const {
        return m_node;
    }

    class SUBSTATE_EXPORT NodeAction : public Action {
    public:
        NodeAction(int type, Node *parent);
        ~NodeAction();

        inline Node *parent() const;

    protected:
        Node *m_parent;
    };

    inline Node *NodeAction::parent() const {
        return m_parent;
    }

    class SUBSTATE_EXPORT RootChangeAction : public Action {
    public:
        RootChangeAction(Node *root, Node *oldRoot);
        ~RootChangeAction();

    public:
        void write(OStream &stream) const override;
        Action *clone() const override;
        void execute(bool undo) override;
        void virtual_hook(int id, void *data) override;

    public:
        inline Node *root() const;
        inline Node *oldRoot() const;

    protected:
        Node *r, *oldr;
    };

    inline Node *RootChangeAction::root() const {
        return r;
    }

    inline Node *RootChangeAction::oldRoot() const {
        return oldr;
    }

}

#endif // NODE_H
