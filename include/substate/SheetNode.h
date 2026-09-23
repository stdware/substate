// Copyright (C) 2022-2025 Stdware Collections (https://www.github.com/stdware)
// SPDX-License-Identifier: Apache-2.0

#ifndef SUBSTATE_SHEETNODE_H
#define SUBSTATE_SHEETNODE_H

#include <map>
#include <memory>
#include <vector>

#include <substate/Action.h>
#include <substate/Node.h>

namespace ss {

    /// A node whose children are addressed by keys that the node assigns.
    ///
    /// Keys are positive and increase with every insertion. A key is never assigned twice, also
    /// if the insertion is undone or its transaction is aborted, so that a key kept by the caller
    /// never refers to a different child.
    class SUBSTATE_EXPORT SheetNode : public Node {
    public:
        inline SheetNode();
        ~SheetNode();

        inline int size() const;

        /// Returns the child with \a key, or \c nullptr if no such child exists.
        Node *at(int key) const;

        /// The keys of the children in ascending order.
        std::vector<int> keys() const;

        /// Inserts \a node, which must be free and without a parent, and returns its key.
        int insert(std::unique_ptr<Node> node);

        /// Removes the child with \a key.
        ///
        /// In a model, the removed child is owned by the action and returns under the same key if
        /// the removal is undone. In a free node, it is destroyed. Use take() to keep it.
        ///
        /// \return whether a child with \a key existed
        bool remove(int key);

        /// Removes and returns the child with \a key, or returns \c nullptr if no such child
        /// exists. The node must be free.
        std::unique_ptr<Node> take(int key);

        /// Moves \a node from its parent in the same model to this node under a new key, keeping
        /// its identity. See TransferAction. Redoing the transfer uses the same key.
        ///
        /// \return the new key, or 0 if the transfer is rejected, creating no action, because
        ///         \a node is the root, its parent is this node, or this node is \a node or one of
        ///         its descendants
        int transferIn(Node *node);

        std::unique_ptr<Node> clone() const override;

    protected:
        inline explicit SheetNode(int type);

        void forEachChild(const std::function<void(Node *)> &func) const override;
        std::unique_ptr<TransferEndpoint> endpointOf(const std::vector<Node *> &children) override;

        /// Copies the children of \a source with their keys, and the key counter, for the clone()
        /// of a subclass. This node must be free and empty.
        void cloneChildrenFrom(const SheetNode &source);

    private:
        std::map<int, std::unique_ptr<Node>> m_children;
        int m_lastKey = 0;

        friend class SheetInsDelAction;
        friend class SheetNodeEndpoint;
    };

    inline SheetNode::SheetNode() : SheetNode(Sheet) {
    }

    inline SheetNode::SheetNode(int type) : Node(type) {
    }

    inline int SheetNode::size() const {
        return int(m_children.size());
    }

    /// Insertion into or removal from a SheetNode.
    ///
    /// Owns the child while it is not in the tree: an insertion before execution and after undo,
    /// a removal after execution.
    class SUBSTATE_EXPORT SheetInsDelAction : public Action {
    public:
        ~SheetInsDelAction();

        inline SheetNode *parent() const;
        inline int key() const;
        inline Node *child() const;

        void forEachHeldNode(const std::function<void(Node *)> &func) const override;

    protected:
        void execute(Operation operation) override;

    private:
        // For an insertion, held is the inserted node. For a removal, held is null and the
        // removed child is read from parent.
        SheetInsDelAction(int type, SheetNode *parent, int key, std::unique_ptr<Node> held);

        SheetNode *m_parent;
        int m_key;
        Node *m_child;
        std::unique_ptr<Node> m_held;

        friend class SheetNode;
    };

    inline SheetNode *SheetInsDelAction::parent() const {
        return m_parent;
    }

    inline int SheetInsDelAction::key() const {
        return m_key;
    }

    inline Node *SheetInsDelAction::child() const {
        return m_child;
    }

}

#endif // SUBSTATE_SHEETNODE_H
