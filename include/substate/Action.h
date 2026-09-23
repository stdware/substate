// Copyright (C) 2022-2025 Stdware Collections (https://www.github.com/stdware)
// SPDX-License-Identifier: Apache-2.0

#ifndef SUBSTATE_ACTION_H
#define SUBSTATE_ACTION_H

#include <functional>
#include <memory>

#include <substate/substate_global.h>

namespace ss {

    class Model;

    class Node;

    /// A recorded change of the tree, which the model can apply and revert.
    ///
    /// An action refers to the nodes it changes by non-owning pointers. It owns a node only in the
    /// cases specified by constraint 2 of docs/Design.md: an insertion owns the inserted nodes
    /// while it is not executed, and a removal owns the removed nodes while it is executed.
    ///
    /// \note The destructor of an action must not access the nodes it refers to without owning
    ///       them, because the actions of a transaction are destroyed in an unspecified order.
    class SUBSTATE_EXPORT Action {
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
            User = 1024,
        };

        /// The occasion on which an action is applied.
        enum Operation {
            /// The first application, within the transaction that creates the action.
            Execute,
            Undo,
            Redo,
        };

        /// Returns whether \a operation applies the change of an action rather than reverting it.
        static inline bool isForward(Operation operation);

        virtual ~Action();

        Action(const Action &) = delete;
        Action &operator=(const Action &) = delete;

        inline int type() const;

        /// Calls \a func on each node that this action owns at present, excluding their
        /// descendants.
        virtual void forEachHeldNode(const std::function<void(Node *)> &func) const;

    protected:
        inline explicit Action(int type);

        /// Applies the action to the tree for \a operation.
        virtual void execute(Operation operation) = 0;

    private:
        int m_type;

        friend class Model;
    };

    inline Action::Action(int type) : m_type(type) {
    }

    inline bool Action::isForward(Operation operation) {
        return operation != Undo;
    }

    inline int Action::type() const {
        return m_type;
    }

    /// Replacement of the root of a model.
    ///
    /// Owns the root that is not in the tree: the new root before execution and after undo, the
    /// old root after execution. Either root may be \c nullptr.
    class SUBSTATE_EXPORT RootChangeAction : public Action {
    public:
        ~RootChangeAction();

        inline Model *model() const;

        /// The root after the action is executed.
        inline Node *newRoot() const;

        /// The root before the action is executed.
        inline Node *oldRoot() const;

        void forEachHeldNode(const std::function<void(Node *)> &func) const override;

    protected:
        void execute(Operation operation) override;

    private:
        RootChangeAction(Model *model, std::unique_ptr<Node> newRoot);

        Model *m_model;
        Node *m_newRoot;
        Node *m_oldRoot;
        std::unique_ptr<Node> m_held;

        friend class Model;
    };

    inline Model *RootChangeAction::model() const {
        return m_model;
    }

    inline Node *RootChangeAction::newRoot() const {
        return m_newRoot;
    }

    inline Node *RootChangeAction::oldRoot() const {
        return m_oldRoot;
    }

}

#endif // SUBSTATE_ACTION_H
