#ifndef ACTION_H
#define ACTION_H

#include <memory>
#include <string>
#include <vector>

#include <substate/sender.h>
#include <substate/stream.h>

namespace Substate {

    class Node;

    class ActionHelper;

    class SUBSTATE_EXPORT Action {
    public:
        inline Action(int type);
        virtual ~Action();

        enum Type {
            BytesReplace = 1,
            BytesInsert,
            BytesRemove,
            VectorInsert,
            VectorMove,
            VectorRemove,
            RecordInsert,
            RecordRemove,
            MappingInsert,
            MappingRemove,
            RootChange,
            User = 1024,
        };

        enum ActionHook {
            CleanNodesHook = 1,
            InsertedNodesHook,
            RemovedNodesHook,
            AcquireInsertedNodesHook,
        };

        inline Type type() const;
        inline int userType() const;

        typedef Action *(*Factory)(IStream &, bool brief);

        static Action *read(IStream &stream, bool brief = false);
        static bool registerFactory(int type, Factory fac);

    public:
        virtual Action *clone() const = 0;
        virtual void execute(bool undo) = 0;

        virtual void virtual_hook(int id, void *data);

    protected:
        int t;

        friend class ActionHelper;

        SUBSTATE_DISABLE_COPY_MOVE(Action)
    };

    inline Action::Action(int type) : t(type) {
    }

    inline Action::Type Action::type() const {
        return t >= User ? User : static_cast<Type>(t);
    }

    inline int Action::userType() const {
        return t;
    }

    class SUBSTATE_EXPORT NodeAction : public Action {
    public:
        NodeAction(int type, Node *parent);
        ~NodeAction();

        inline Node *parent() const;

    protected:
        Node *m_parent;
    };

    Node *NodeAction::parent() const {
        return m_parent;
    }

    class SUBSTATE_EXPORT ActionNotification : public Notification {
    public:
        ActionNotification(Type type, Action *action);
        ~ActionNotification();

        inline Action *action() const;

    protected:
        Action *a;
    };

    Action *ActionNotification::action() const {
        return a;
    }

}

#endif // ACTION_H
