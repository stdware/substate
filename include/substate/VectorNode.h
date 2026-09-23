// Copyright (C) 2022-2025 Stdware Collections (https://www.github.com/stdware)
// SPDX-License-Identifier: Apache-2.0

#ifndef SUBSTATE_VECTORNODE_H
#define SUBSTATE_VECTORNODE_H

#include <memory>
#include <vector>

#include <substate/Action.h>
#include <substate/Node.h>

namespace ss {

    /// A node with an ordered list of children, addressed by index.
    class SUBSTATE_EXPORT VectorNode : public Node {
    public:
        inline VectorNode();
        ~VectorNode();

        inline int size() const;
        inline Node *at(int index) const;

        /// Inserts \a nodes before \a index. Each node must be free and without a parent.
        void insert(int index, std::vector<std::unique_ptr<Node>> nodes);
        inline void insert(int index, std::unique_ptr<Node> node);
        inline void append(std::vector<std::unique_ptr<Node>> nodes);
        inline void append(std::unique_ptr<Node> node);

        /// Removes \a count children starting at \a index.
        ///
        /// In a model, the removed children are owned by the action and return to this node if the
        /// removal is undone. In a free node, they are destroyed. Use take() to keep them.
        void remove(int index, int count);

        /// Moves \a count children starting at \a index so that the first of them is at
        /// \a destination afterwards. \a destination is an index into the list after the move and
        /// must differ from \a index.
        void move(int index, int count, int destination);

        /// Removes and returns \a count children starting at \a index. The node must be free.
        std::vector<std::unique_ptr<Node>> take(int index, int count);

        /// Moves \a nodes from their parent in the same model to this node before \a index,
        /// keeping their identity. The nodes must be children of one parent, and consecutive in
        /// order if that parent is a VectorNode. See TransferAction.
        ///
        /// \return whether the transfer was performed. It is rejected, creating no action, if a
        ///         node is the root, if the parent is this node, or if this node is one of the
        ///         nodes or their descendants.
        bool transferIn(int index, const std::vector<Node *> &nodes);
        inline bool transferIn(int index, Node *node);

        std::unique_ptr<Node> clone() const override;

    protected:
        inline explicit VectorNode(int type);

        void forEachChild(const std::function<void(Node *)> &func) const override;
        std::unique_ptr<TransferEndpoint> endpointOf(const std::vector<Node *> &children) override;

        /// Appends copies of the children of \a source, for the clone() of a subclass. This node
        /// must be free and empty.
        void cloneChildrenFrom(const VectorNode &source);

    private:
        std::vector<std::unique_ptr<Node>> m_children;

        friend class VectorInsDelAction;
        friend class VectorMoveAction;
        friend class VectorNodeEndpoint;
    };

    inline VectorNode::VectorNode() : VectorNode(Vector) {
    }

    inline VectorNode::VectorNode(int type) : Node(type) {
    }

    inline int VectorNode::size() const {
        return int(m_children.size());
    }

    inline Node *VectorNode::at(int index) const {
        return m_children.at(size_t(index)).get();
    }

    inline void VectorNode::insert(int index, std::unique_ptr<Node> node) {
        std::vector<std::unique_ptr<Node>> nodes;
        nodes.push_back(std::move(node));
        insert(index, std::move(nodes));
    }

    inline void VectorNode::append(std::vector<std::unique_ptr<Node>> nodes) {
        insert(size(), std::move(nodes));
    }

    inline void VectorNode::append(std::unique_ptr<Node> node) {
        insert(size(), std::move(node));
    }

    inline bool VectorNode::transferIn(int index, Node *node) {
        return transferIn(index, std::vector<Node *>{node});
    }

    /// Insertion into or removal from a VectorNode.
    ///
    /// Owns the children while they are not in the tree: an insertion before execution and after
    /// undo, a removal after execution.
    class SUBSTATE_EXPORT VectorInsDelAction : public Action {
    public:
        ~VectorInsDelAction();

        inline VectorNode *parent() const;
        inline int index() const;

        /// The inserted or removed children in order.
        inline const std::vector<Node *> &children() const;

        /// Returns whether applying the action for \a operation inserts the children rather than
        /// removing them.
        inline bool isInsertion(Operation operation = Execute) const;

        void forEachHeldNode(const std::function<void(Node *)> &func) const override;

    protected:
        void execute(Operation operation) override;

    private:
        // For an insertion, held contains the inserted nodes. For a removal, held is empty and
        // the removed children are read from parent.
        VectorInsDelAction(int type, VectorNode *parent, int index, int count,
                           std::vector<std::unique_ptr<Node>> held);

        VectorNode *m_parent;
        int m_index;
        std::vector<Node *> m_children;
        std::vector<std::unique_ptr<Node>> m_held;

        friend class VectorNode;
    };

    inline VectorNode *VectorInsDelAction::parent() const {
        return m_parent;
    }

    inline int VectorInsDelAction::index() const {
        return m_index;
    }

    inline const std::vector<Node *> &VectorInsDelAction::children() const {
        return m_children;
    }

    inline bool VectorInsDelAction::isInsertion(Operation operation) const {
        return (type() == VectorInsert) == isForward(operation);
    }

    /// Reordering of children within a VectorNode. Owns no node.
    class SUBSTATE_EXPORT VectorMoveAction : public Action {
    public:
        ~VectorMoveAction();

        inline VectorNode *parent() const;

        /// The index of the first moved child before the action is applied for \a operation.
        inline int index(Operation operation = Execute) const;

        inline int count() const;

        /// The index of the first moved child after the action is applied for \a operation.
        inline int destination(Operation operation = Execute) const;

    protected:
        void execute(Operation operation) override;

    private:
        VectorMoveAction(VectorNode *parent, int index, int count, int destination);

        VectorNode *m_parent;
        int m_index;
        int m_count;
        int m_destination;

        friend class VectorNode;
    };

    inline VectorNode *VectorMoveAction::parent() const {
        return m_parent;
    }

    inline int VectorMoveAction::index(Operation operation) const {
        return isForward(operation) ? m_index : m_destination;
    }

    inline int VectorMoveAction::count() const {
        return m_count;
    }

    inline int VectorMoveAction::destination(Operation operation) const {
        return isForward(operation) ? m_destination : m_index;
    }

}

#endif // SUBSTATE_VECTORNODE_H
