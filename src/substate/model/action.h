#ifndef ACTION_H
#define ACTION_H

#include <memory>
#include <string>
#include <vector>
#include <unordered_map>

#include <substate/sender.h>
#include <substate/stream.h>

namespace Substate {

    class Node;

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
            SheetInsert,
            SheetRemove,
            MappingAssign,
            StructAssign,
            RootChange,
            User = 1024,
        };

        enum ActionHook {
            CleanNodesHook = 1,
            InsertedNodesHook,
            RemovedNodesHook,
        };

        inline int type() const;

        typedef Action *(*Factory)(IStream &, const std::unordered_map<int, Node *> &);

        static Action *read(IStream &stream, const std::unordered_map<int, Node *> &existingNodes);
        static bool registerFactory(int type, Factory fac);

    public:
        virtual void write(OStream &stream) const = 0;
        virtual Action *clone() const = 0;
        virtual void execute(bool undo) = 0;
        virtual void virtual_hook(int id, void *data);

    protected:
        int t;

        QMSETUP_DISABLE_COPY_MOVE(Action)
    };

    inline Action::Action(int type) : t(type) {
    }

    inline int Action::type() const {
        return t;
    }

    class SUBSTATE_EXPORT ActionNotification : public Notification {
    public:
        ActionNotification(Type type, Action *action);
        ~ActionNotification();

        inline Action *action() const;

    protected:
        Action *a;
    };

    inline Action *ActionNotification::action() const {
        return a;
    }

}

#endif // ACTION_H
