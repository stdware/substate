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

    class Action {
    public:
        enum Type {
            RootChange = 1,
            VectorInsert,
            VectorRemove,
            VectorMove,
            SheetInsert,
            SheetRemove,
            BytesReplace,
            BytesInsert,
            BytesRemove,
            MappingAssign,
            StructAssign,
        };

        /// Default constructor creates an invalid action.
        inline explicit Action(int type);
        virtual ~Action() = default;

        inline int type() const;

        inline int state() const;
        inline void setState(int state);

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
        inline ActionNotification(Type type, const Action *action);
        ~ActionNotification() = default;

        inline const Action *action() const;

    protected:
        const Action *_action;
    };

    inline const Action *ActionNotification::action() const {
        return _action;
    }

    ActionNotification::ActionNotification(Type type, const Action *action)
        : Notification(type), _action(action) {
    }


    /// ActionIOInterface - Interface for reading and writing actions and nodes.
    class ActionIOInterface {
    public:
        virtual ~ActionIOInterface() = default;

        virtual std::shared_ptr<Node> readNode(std::istream &is) = 0;
        virtual void writeNode(const Node &node, std::ostream &os) = 0;

        virtual std::unique_ptr<Action> readAction(std::istream &is) = 0;
        virtual void writeAction(const Action &action, std::ostream &os) = 0;
    };

}

#endif // SUBSTATE_ACTION_H