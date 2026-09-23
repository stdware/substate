// Copyright (C) 2022-2025 Stdware Collections (https://www.github.com/stdware)
// SPDX-License-Identifier: Apache-2.0

#ifndef SUBSTATE_NODE_H
#define SUBSTATE_NODE_H

#include <cstdint>
#include <functional>
#include <memory>

#include <substate/substate_global.h>

namespace ss {

    class Model;

    class NodePrivate;

    /// A node of a document tree.
    ///
    /// A node has exactly one owner at any time. A free node, which has not entered a model, is
    /// owned by the caller. A node in the tree is owned by its parent, or by the model if it is
    /// the root. A node removed from the tree is owned by the action that removed it, and a node
    /// whose insertion was undone is owned by the inserting action, until that action is
    /// discarded from the history. See docs/Design.md for the rules and their proof.
    ///
    /// A free node can be modified at any time without creating actions. A node that has entered
    /// a model can be modified only while it is in the tree and the model is in a transaction.
    class SUBSTATE_EXPORT Node {
    public:
        enum Type {
            Bytes,
            Vector,
            Sheet,
            Mapping,
            Struct,
            User = 1024,
        };

        virtual ~Node();

        Node(const Node &) = delete;
        Node &operator=(const Node &) = delete;

        inline int type() const;

        /// The identifier assigned when the node entered a model, or 0 for a free node. It is
        /// unique within the model and remains unchanged until the node is destroyed.
        inline std::uint64_t id() const;

        /// The parent, or \c nullptr for the root, for the root of a removed subtree, and for a
        /// free node that has not been inserted into another free node.
        inline Node *parent() const;

        /// The model, or \c nullptr for a free node.
        inline Model *model() const;

        /// Returns whether the node has not entered a model.
        inline bool isFree() const;

        /// Returns whether the node and all its ancestors are in the tree of its model.
        inline bool isAttached() const;

        /// Returns a copy of this node and its descendants as free nodes without identifiers.
        ///
        /// \note A subclass must override this function to create an object of its own type.
        virtual std::unique_ptr<Node> clone() const = 0;

    protected:
        inline explicit Node(int type);

        /// Calls \a func on each child, excluding further descendants.
        virtual void forEachChild(const std::function<void(Node *)> &func) const;

        /// Returns whether the node can be modified: it is free, or it is in the tree and the
        /// model is in a transaction.
        bool isWritable() const;

    private:
        int m_type;
        std::uint64_t m_id = 0;
        Node *m_parent = nullptr;
        Model *m_model = nullptr;
        bool m_attached = false;

        friend class NodePrivate;
    };

    inline Node::Node(int type) : m_type(type) {
    }

    inline int Node::type() const {
        return m_type;
    }

    inline std::uint64_t Node::id() const {
        return m_id;
    }

    inline Node *Node::parent() const {
        return m_parent;
    }

    inline Model *Node::model() const {
        return m_model;
    }

    inline bool Node::isFree() const {
        return m_model == nullptr;
    }

    inline bool Node::isAttached() const {
        return m_attached;
    }

}

#endif // SUBSTATE_NODE_H
