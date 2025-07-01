// Copyright (C) 2022-2025 Stdware Collections (https://www.github.com/stdware)
// SPDX-License-Identifier: Apache-2.0

#ifndef SUBSTATE_ACTION_H
#define SUBSTATE_ACTION_H

#include <memory>
#include <functional>
#include <iostream>

#include <substate/Notification.h>
#include <substate/Node.h>

namespace ss {

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

        /// Query the nodes associated with the action.
        virtual void queryNodes( //
            bool inserted, const std::function<void(const std::shared_ptr<Node> &)> &add) = 0;

        /// Undo or redo the action.
        virtual void execute(bool undo) = 0;

    protected:
        int _type;
    };

    inline Action::Action(int type) : _type(type) {
    }

    inline int Action::type() const {
        return _type;
    }


    /// NodeAction - Base action for node change.
    class NodeAction : public Action {
    public:
        inline NodeAction(int type, const std::shared_ptr<Node> &parent);
        ~NodeAction() = default;

        inline std::shared_ptr<Node> parent() const;

    protected:
        std::shared_ptr<Node> _parent;
    };

    inline NodeAction::NodeAction(int type, const std::shared_ptr<Node> &parent)
        : Action(type), _parent(parent) {
    }

    inline std::shared_ptr<Node> NodeAction::parent() const {
        return _parent;
    }


    /// RootChangeAction - Action for model root change.
    class SUBSTATE_EXPORT RootChangeAction : public Action {
    public:
        inline RootChangeAction(const std::shared_ptr<Node> &oldRoot,
                                const std::shared_ptr<Node> &newRoot);
        ~RootChangeAction() = default;

        std::unique_ptr<Action> clone(bool detach) const override;
        void queryNodes(bool inserted,
                        const std::function<void(const std::shared_ptr<Node> &)> &add) override;
        void execute(bool undo) override;

    public:
        inline std::shared_ptr<Node> root() const;
        inline std::shared_ptr<Node> oldRoot() const;

    protected:
        std::shared_ptr<Node> _oldRoot;
        std::shared_ptr<Node> _newRoot;
    };

    inline RootChangeAction::RootChangeAction(const std::shared_ptr<Node> &oldRoot,
                                              const std::shared_ptr<Node> &newRoot)
        : Action(Action::RootChange), _oldRoot(oldRoot), _newRoot(newRoot) {
    }

    inline std::shared_ptr<Node> RootChangeAction::root() const {
        return _newRoot;
    }

    inline std::shared_ptr<Node> RootChangeAction::oldRoot() const {
        return _oldRoot;
    }


    /// ActionNotification - Notification carrying an action.
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


    /// ActionReader - Action deserialize interface.
    class ActionReader {
    public:
        inline ActionReader(std::istream &is);
        virtual ~ActionReader() = default;

        inline std::istream &in() const;

        virtual std::shared_ptr<Action> readOne() const;

    protected:
        std::istream &_in;
    };

    inline ActionReader::ActionReader(std::istream &in) : _in(in) {
    }

    inline std::istream &ActionReader::in() const {
        return _in;
    }


    /// ActionWriter - Node serialize interface.
    class ActionWriter {
    public:
        inline ActionWriter(std::ostream &os);
        virtual ~ActionWriter() = default;

        inline std::ostream &out() const;

        virtual void witeOne(const std::shared_ptr<Action> &action) const = 0;

    protected:
        std::ostream &_out;
    };

    inline ActionWriter::ActionWriter(std::ostream &os) : _out(os) {
    }

    inline std::ostream &ActionWriter::out() const {
        return _out;
    }

}

#endif // SUBSTATE_ACTION_H