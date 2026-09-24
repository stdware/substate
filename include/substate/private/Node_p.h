// Copyright (C) 2022-present Stdware Collections (https://www.github.com/stdware)
// SPDX-License-Identifier: Apache-2.0

#ifndef SUBSTATE_NODE_P_H
#define SUBSTATE_NODE_P_H

#include <cstdint>
#include <functional>
#include <memory>
#include <utility>
#include <vector>

#include <substate/Action.h>
#include <substate/Node.h>
#include <substate/private/Transfer_p.h>

namespace ss {

    /// Operations on the internal state of nodes and models, shared by the node types and their
    /// actions, including those of qsubstate.
    class SUBSTATE_EXPORT NodePrivate {
    public:
        /// Calls \a func on \a node and each of its descendants, parents before children.
        static void forEachInSubtree(Node *node, const std::function<void(Node *)> &func);

        /// Returns whether \a node can be inserted into another node or become the root: it is
        /// free and has no parent. See constraint 1 in docs/Design.md.
        static inline bool isInsertable(const Node *node);

        /// Returns whether \a node is \a target or one of its ancestors. Inserting \a node into
        /// \a target would then create a cycle.
        static inline bool isAncestorOrSelf(const Node *node, const Node *target);

        /// Records \a parent as the parent of the free node \a node, which is inserted into a free
        /// container without an action.
        static inline void setFreeParent(Node *node, Node *parent);

        /// Attaches \a node to \a parent in the tree of \a model, or makes it the root if
        /// \a parent is \c nullptr. A free subtree enters the model first: each of its nodes
        /// receives the model and an identifier. Every node of the subtree becomes attached.
        static void attach(Node *node, Node *parent, Model *model);

        /// Takes \a node off its parent. Every node of the subtree becomes detached. The model and
        /// the identifiers remain.
        static void detach(Node *node);

        /// Records \a parent as the parent of \a node, which remains in the tree. Used by transfer.
        static inline void reparent(Node *node, Node *parent);

        /// Transfers \a nodes, children of one parent, to the position \a targetEnd of \a target
        /// within the current transaction.
        ///
        /// \return whether the transfer was performed. A rejected transfer creates no action. The
        ///         transfer is rejected if a node has no parent, if the parent is \a target, if
        ///         \a target is one of the nodes or their descendants, or if the parent cannot
        ///         provide a position for the nodes together.
        static bool transfer(Node *target, std::unique_ptr<TransferEndpoint> targetEnd,
                             const std::vector<Node *> &nodes);

        /// Applies \a action for Action::Execute with the notifications of the observers of
        /// \a model, and appends it to the transaction in progress.
        static void execute(Model *model, std::unique_ptr<Action> action);

        /// Emits the notifications of the destruction of the nodes that \a action owns, which is
        /// about to be destroyed. Called by the owner of an action before destroying it.
        static void aboutToDiscard(const Action &action);

        /// Removes \a node from the identifier index of its model. Called by the destructor.
        static void removeFromIndex(Node *node);

        /// Makes the free nodes of \a nodes enter \a model with the paired identifiers, without
        /// entering its tree, and raises the identifier counter of the model above them.
        ///
        /// \return whether the nodes entered the model. They do not if an identifier is 0,
        ///         occurs twice, or exists in the model, and then no node enters.
        static bool adopt(Model *model, const std::vector<std::pair<Node *, std::uint64_t>> &nodes);

        /// Call the protected functions of \a node with the same names, for the codec.
        static inline void writeContent(const Node *node, Encoder &encoder);
        static inline bool readContent(Node *node, Decoder &decoder);
        static inline std::unique_ptr<TransferEndpoint> readEndpoint(Node *node, Decoder &decoder);

        /// Returns whether the arguments of an insertion into a sequence of \a size elements are
        /// valid.
        static inline bool isValidInsertion(int index, int size);

        /// Returns whether the arguments of a removal from a sequence of \a size elements are
        /// valid.
        static inline bool isValidRemoval(int index, int count, int size);
    };

    inline bool NodePrivate::isInsertable(const Node *node) {
        return node && node->isFree() && !node->parent();
    }

    inline bool NodePrivate::isAncestorOrSelf(const Node *node, const Node *target) {
        for (auto current = target; current; current = current->parent()) {
            if (current == node) {
                return true;
            }
        }
        return false;
    }

    inline void NodePrivate::setFreeParent(Node *node, Node *parent) {
        node->m_parent = parent;
    }

    inline void NodePrivate::reparent(Node *node, Node *parent) {
        node->m_parent = parent;
    }

    inline void NodePrivate::writeContent(const Node *node, Encoder &encoder) {
        node->writeContent(encoder);
    }

    inline bool NodePrivate::readContent(Node *node, Decoder &decoder) {
        return node->readContent(decoder);
    }

    inline std::unique_ptr<TransferEndpoint> NodePrivate::readEndpoint(Node *node,
                                                                       Decoder &decoder) {
        return node->readEndpoint(decoder);
    }

    inline bool NodePrivate::isValidInsertion(int index, int size) {
        return index >= 0 && index <= size;
    }

    inline bool NodePrivate::isValidRemoval(int index, int count, int size) {
        return index >= 0 && count > 0 && index <= size - count;
    }

}

#endif // SUBSTATE_NODE_P_H
