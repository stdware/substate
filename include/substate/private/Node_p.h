// Copyright (C) 2022-2025 Stdware Collections (https://www.github.com/stdware)
// SPDX-License-Identifier: Apache-2.0

#ifndef SUBSTATE_NODE_P_H
#define SUBSTATE_NODE_P_H

#include <memory>

#include <substate/Action.h>
#include <substate/Node.h>

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

        /// Hangs \a node under \a parent in the tree of \a model, or makes it the root if
        /// \a parent is \c nullptr. A free subtree enters the model first: each of its nodes
        /// receives the model and an identifier. Every node of the subtree becomes attached.
        static void attach(Node *node, Node *parent, Model *model);

        /// Takes \a node off its parent. Every node of the subtree becomes detached. The model and
        /// the identifiers remain.
        static void detach(Node *node);

        /// Appends \a action, which has been executed, to the transaction in progress.
        static void pushAction(Model *model, std::unique_ptr<Action> action);

        /// Removes \a node from the identifier index of its model. Called by the destructor.
        static void removeFromIndex(Node *node);

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

    inline bool NodePrivate::isValidInsertion(int index, int size) {
        return index >= 0 && index <= size;
    }

    inline bool NodePrivate::isValidRemoval(int index, int count, int size) {
        return index >= 0 && count > 0 && index <= size - count;
    }

}

#endif // SUBSTATE_NODE_P_H
