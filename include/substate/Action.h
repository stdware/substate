// Copyright (C) 2022-2025 Stdware Collections (https://www.github.com/stdware)
// SPDX-License-Identifier: Apache-2.0

#ifndef SUBSTATE_ACTION_H
#define SUBSTATE_ACTION_H

#include <functional>
#include <memory>
#include <vector>

#include <substate/substate_global.h>

namespace ss {

    class Codec;

    class Decoder;

    class Encoder;

    class Model;

    class Node;

    class TransferEndpoint;

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
            Transfer,
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
        ///
        /// The accessors of the action types that take an Operation describe the change that
        /// applying the action for that operation makes. Applying a removal for Undo inserts, for
        /// example. The default Execute describes the change as recorded.
        enum Operation {
            /// The first application, within the transaction that creates the action.
            Execute,
            Undo,
            Redo,
        };

        /// Whether the change of an action is present in the tree, which is the case exactly if
        /// the action precedes the current position of the history. The state determines the
        /// nodes that the action owns, see constraint 2 in docs/Design.md.
        enum State {
            /// The action precedes the current position. It was executed or redone last.
            Applied,
            /// The action follows the current position. It was undone last, or has not been
            /// executed yet, as an action read from a log for replay.
            Unapplied,
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

        /// Writes the action, excluding its type, for Encoder::writeAction(). An action type that
        /// can be persisted overrides this function and registers a reader with the Codec. The
        /// default records a failure in \a encoder.
        virtual void write(Encoder &encoder) const;

    private:
        int m_type;

        friend class Encoder;
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

        /// The root after the action is applied for \a operation.
        inline Node *newRoot(Operation operation = Execute) const;

        /// The root before the action is applied for \a operation.
        inline Node *oldRoot(Operation operation = Execute) const;

        void forEachHeldNode(const std::function<void(Node *)> &func) const override;

    protected:
        void execute(Operation operation) override;
        void write(Encoder &encoder) const override;

    private:
        // held is the root that is not in the tree: newRoot while unapplied, oldRoot while
        // applied.
        RootChangeAction(Model *model, Node *newRoot, Node *oldRoot, std::unique_ptr<Node> held);

        static std::unique_ptr<Action> read(Decoder &decoder, State state);

        Model *m_model;
        Node *m_newRoot;
        Node *m_oldRoot;
        std::unique_ptr<Node> m_held;

        friend class Codec;
        friend class Model;
    };

    inline Model *RootChangeAction::model() const {
        return m_model;
    }

    inline Node *RootChangeAction::newRoot(Operation operation) const {
        return isForward(operation) ? m_newRoot : m_oldRoot;
    }

    inline Node *RootChangeAction::oldRoot(Operation operation) const {
        return isForward(operation) ? m_oldRoot : m_newRoot;
    }

    /// Transfer of nodes from one parent to another within the same model, which keeps their
    /// addresses and identifiers. Owns no node, because the nodes remain in the tree.
    ///
    /// Created by the \c transferIn() functions of the container node types.
    ///
    /// The positions of the nodes are read from the containers: the nodes are at their source
    /// positions in ModelObserver::actionAboutToApply(), and at their target positions in
    /// ModelObserver::actionApplied().
    class SUBSTATE_EXPORT TransferAction : public Action {
    public:
        ~TransferAction();

        /// The parent of the nodes before the action is applied for \a operation.
        Node *source(Operation operation = Execute) const;

        /// The parent of the nodes after the action is applied for \a operation.
        Node *target(Operation operation = Execute) const;

        /// The transferred nodes in order.
        inline const std::vector<Node *> &nodes() const;

    protected:
        void execute(Operation operation) override;
        void write(Encoder &encoder) const override;

    private:
        TransferAction(std::unique_ptr<TransferEndpoint> source,
                       std::unique_ptr<TransferEndpoint> target, std::vector<Node *> nodes);

        static std::unique_ptr<Action> read(Decoder &decoder, State state);

        std::unique_ptr<TransferEndpoint> m_source;
        std::unique_ptr<TransferEndpoint> m_target;
        std::vector<Node *> m_nodes;

        friend class Codec;
        friend class NodePrivate;
    };

    inline const std::vector<Node *> &TransferAction::nodes() const {
        return m_nodes;
    }

}

#endif // SUBSTATE_ACTION_H
