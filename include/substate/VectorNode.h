// Copyright (C) 2022-2025 Stdware Collections (https://www.github.com/stdware)
// SPDX-License-Identifier: Apache-2.0

#ifndef SUBSTATE_VECTORNODE_H
#define SUBSTATE_VECTORNODE_H

#include <vector>

#include <substate/Node.h>
#include <substate/Action.h>

namespace ss {

    class VectorInsDelAction;

    class VectorMoveAction;

    class VectorNodePrivate;

    /// VectorNode - Vector data structure node.
    class SUBSTATE_EXPORT VectorNode : public Node {
    public:
        inline explicit VectorNode(int type = Vector);
        ~VectorNode();

    public:
        inline void prepend(NodePtr node);
        inline void prepend(std::vector<NodePtr> nodes);
        inline void append(NodePtr node);
        inline void append(std::vector<NodePtr> nodes);
        inline void insert(int index, NodePtr node);
        inline void removeOne(int index);
        void insert(int index, std::vector<NodePtr> nodes);
        void move(int index, int count, int dest);         // dest: destination index before move
        inline void move2(int index, int count, int dest); // dest: destination index after move
        void remove(int index, int count);
        inline Node *at(int index) const;
        inline const std::vector<NodePtr> &data() const;
        inline int count() const;
        inline int size() const;

    protected:
        NodePtr clone(bool copyId) const override;
        void propagateChildren(const std::function<void(Node *)> &func) override;

        std::vector<NodePtr> _vec;

        friend class VectorNodePrivate;
        friend class VectorInsDelAction;
        friend class VectorMoveAction;
    };

    inline VectorNode::VectorNode(int type) : Node(type) {
    }

    inline void VectorNode::prepend(NodePtr node) {
        insert(0, std::move(node));
    }

    inline void VectorNode::prepend(std::vector<NodePtr> nodes) {
        insert(0, std::move(nodes));
    }

    inline void VectorNode::append(NodePtr node) {
        insert(size(), std::move(node));
    }

    inline void VectorNode::append(std::vector<NodePtr> nodes) {
        insert(size(), std::move(nodes));
    }

    inline void VectorNode::insert(int index, NodePtr node) {
        insert(index, std::vector<NodePtr>{std::move(node)});
    }

    inline void VectorNode::removeOne(int index) {
        remove(index, 1);
    }

    inline void VectorNode::move2(int index, int count, int dest) {
        move(index, count, (dest <= index) ? dest : (dest + count));
    }

    inline Node *VectorNode::at(int index) const {
        return _vec.at(index).get();
    }

    inline const std::vector<NodePtr> &VectorNode::data() const {
        return _vec;
    }

    inline int VectorNode::count() const {
        return size();
    }

    inline int VectorNode::size() const {
        return int(_vec.size());
    }


    /// VectorAction - Action for \c VectorNode operations.
    class VectorAction : public NodeAction {
    public:
        inline VectorAction(Type type, Node *parent, int index);
        ~VectorAction() = default;

    public:
        inline int index() const;

    public:
        int _index;
    };

    inline int VectorAction::index() const {
        return _index;
    }

    inline VectorAction::VectorAction(Type type, Node *parent, int index)
        : NodeAction(type, parent), _index(index) {
    }


    /// VectorMoveAction - Action for \c VectorNode movement.
    class SUBSTATE_EXPORT VectorMoveAction : public VectorAction {
    public:
        inline VectorMoveAction(Node *parent, int index, int count, int dest);
        ~VectorMoveAction() = default;

    public:
        void queryNodes(bool inserted, const std::function<void(const NodePtr &)> &add) override;
        void execute(bool undo) override;

    public:
        inline int count() const;
        inline int destination() const;

    protected:
        int _count, _dest;
    };

    inline VectorMoveAction::VectorMoveAction(Node *parent, int index, int count, int dest)
        : VectorAction(VectorMove, parent, index), _count(count), _dest(dest) {
    }

    inline int VectorMoveAction::count() const {
        return _count;
    }

    inline int VectorMoveAction::destination() const {
        return _dest;
    }


    /// VectorInsDelAction - Action for \c VectorNode insertion or deletion.
    class SUBSTATE_EXPORT VectorInsDelAction : public VectorAction {
    public:
        inline VectorInsDelAction(Type type, Node *parent, int index,
                                  std::vector<NodePtr> children);
        ~VectorInsDelAction() = default;

    public:
        void queryNodes(bool inserted, const std::function<void(const NodePtr &)> &add) override;
        void execute(bool undo) override;

    public:
        inline const std::vector<NodePtr> &children() const;

    protected:
        std::vector<NodePtr> _children;
    };

    inline VectorInsDelAction::VectorInsDelAction(Type type, Node *parent, int index,
                                                  std::vector<NodePtr> children)
        : VectorAction(type, parent, index), _children(std::move(children)) {
    }

    inline const std::vector<NodePtr> &VectorInsDelAction::children() const {
        return _children;
    }

}

#endif // SUBSTATE_VECTORNODE_H
