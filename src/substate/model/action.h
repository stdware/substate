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

        enum State {
            Normal,
            Unreferenced,
            Detached,
            Deleted,
        };

        enum ActionHook {
            DetachHook = 1,
            InsertedNodesHook,
            RemovedNodesHook,
            DeferredReferenceHook,
        };

        inline int type() const;

        inline int state() const;
        inline void setState(State state);

        typedef Action *(*Factory)(IStream &);

        static Action *read(IStream &stream);
        static bool registerFactory(int type, Factory fac);
        inline void writeWithType(OStream &stream) const;

    public:
        // internal APIs, should not be called by user code
        void detach();
        void deferredReference(const std::unordered_map<int, Node *> &existingItems);

    public:
        virtual void write(OStream &stream) const = 0;
        virtual Action *clone() const = 0;
        virtual void execute(bool undo) = 0;
        virtual void virtual_hook(int id, void *data);

    protected:
        int t;
        int s;

        SUBSTATE_DISABLE_COPY_MOVE(Action)
    };

    inline Action::Action(int type) : t(type), s(Normal) {
    }

    inline int Action::type() const {
        return t;
    }

    inline int Action::state() const {
        return s;
    }

    inline void Action::setState(Substate::Action::State state) {
        s = state;
    }

    inline void Action::writeWithType(OStream &stream) const {
        stream << t;
        write(stream);
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
