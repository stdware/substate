#ifndef SUBSTATE_ACTION_H
#define SUBSTATE_ACTION_H

#include <memory>
#include <functional>
#include <iostream>

#include <substate/Notification.h>

namespace ss {

    class Node;

    class ActionReader;

    class Action {
    public:
        enum Type {
            BytesReplace = 1,
            BytesInsert,
            BytesRemove,
            VectorInsert,
            VectorMove,
            VectorRemove,
            SheetInsert,
            SheetRemove,
            RootChange,
            User = 1024,
        };

        enum State {
            Allocated,
            BeforeInitialize,
            Initialized,
        };

        /// Default constructor creates an invalid action.
        inline explicit Action(int type);
        virtual ~Action() = default;

        Action(const Action &) = delete;
        Action &operator=(const Action &) = delete;

        inline int type() const;

        inline int state() const;
        inline void setState(int state);

        /// Clone the action.
        /// \param detach If true, the associated nodes should be assigned as the cloned nodes.
        virtual std::unique_ptr<Action> clone(bool detach) const = 0;

        /// Serialize the action to a stream.
        /// \note The \c type should not be written, as they are used to determine the constructor
        /// and already written by the caller.
        virtual void write(std::ostream &os) const = 0;

        /// Query the nodes associated with the action.
        virtual void queryNodes( //
            bool inserted, const std::function<void(const std::shared_ptr<Node> &)> &add) = 0;

        /// Undo or redo the action.
        virtual void execute(bool undo) = 0;

        /// Read the action in \c Allocated state from a stream, and change to \c BeforeInitialize
        /// state.
        virtual void read(std::istream &is) = 0;

        /// Initialize the action, and change to the \c Initialized state.
        virtual void initialize( //
            const std::function<std::shared_ptr<Node>(size_t /*id*/)> &find) = 0;

    public:
        /// Serialize the action with the preceding \c type enum.
        SUBSTATE_EXPORT static void write(Action &action, std::ostream &os);

    protected:
        int _type;
        int _state;
    };

    inline Action::Action(int type) : _type(type), _state(Allocated) {
    }

    inline int Action::type() const {
        return _type;
    }

    inline int Action::state() const {
        return _state;
    }

    inline void Action::setState(int state) {
        _state = state;
    }

    class ActionNotification : public Notification {
    public:
        inline ActionNotification(Type type, Action *action);
        ~ActionNotification() = default;

        inline Action *action() const;

    protected:
        Action *_action;
    };

    inline Action *ActionNotification::action() const {
        return _action;
    }

    ActionNotification::ActionNotification(Type type, Action *action)
        : Notification(type), _action(action) {
    }

    class SUBSTATE_EXPORT ActionReader {
    public:
        virtual ~ActionReader() = default;

        std::unique_ptr<Action> readAction(std::istream &is);

    protected:
        /// Returns a new action deserialized from \a is with the \a type.
        virtual std::unique_ptr<Action> readAction(int type, std::istream &is) const = 0;
    };

}

#endif // SUBSTATE_ACTION_H