// Copyright (C) 2022-2025 Stdware Collections (https://www.github.com/stdware)
// SPDX-License-Identifier: Apache-2.0

#ifndef SUBSTATE_VECTORNODE_H
#define SUBSTATE_VECTORNODE_H

#include <vector>

#include <substate/Node.h>
#include <substate/Action.h>
#include <substate/ArrayView.h>

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
        inline void prepend(const std::shared_ptr<Node> &node);
        inline void prepend(std::vector<std::shared_ptr<Node>> nodes);
        inline void append(const std::shared_ptr<Node> &node);
        inline void append(std::vector<std::shared_ptr<Node>>nodes);
        inline void insert(int index, const std::shared_ptr<Node> &node);
        inline void removeOne(int index);
        void insert(int index, std::vector<std::shared_ptr<Node>>nodes);
        void move(int index, int count, int dest);         // dest: destination index before move
        inline void move2(int index, int count, int dest); // dest: destination index after move
        void remove(int index, int count);
        inline std::shared_ptr<Node> at(int index) const;
        inline ArrayView<std::shared_ptr<Node>> data() const;
        inline int count() const;
        inline int size() const;

    protected:
        std::shared_ptr<Node> clone(bool copyId) const override;
        void propagateChildren(const std::function<void(Node *)> &func) override;

        std::vector<std::shared_ptr<Node>> _vec;

        friend class VectorNodePrivate;
        friend class VectorInsDelAction;
        friend class VectorMoveAction;
    };

    inline VectorNode::VectorNode(int type) : Node(type) {
    }

    inline void VectorNode::prepend(const std::shared_ptr<Node> &node) {
        insert(0, node);
    }

    inline void VectorNode::prepend(std::vector<std::shared_ptr<Node>>nodes) {
        insert(0, std::move(nodes));
    }

    inline void VectorNode::append(const std::shared_ptr<Node> &node) {
        insert(size(), node);
    }

    inline void VectorNode::append(std::vector<std::shared_ptr<Node>>nodes) {
        insert(size(), std::move(nodes));
    }

    inline void VectorNode::insert(int index, const std::shared_ptr<Node> &node) {
        insert(index, std::vector<std::shared_ptr<Node>>{node});
    }

    inline void VectorNode::removeOne(int index) {
        remove(index, 1);
    }

    inline void VectorNode::move2(int index, int count, int dest) {
        move(index, count, (dest <= index) ? dest : (dest + count));
    }

    inline std::shared_ptr<Node> VectorNode::at(int index) const {
        return _vec.at(index);
    }

    inline ArrayView<std::shared_ptr<Node>> VectorNode::data() const {
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
        inline VectorAction(Type type, const std::shared_ptr<Node> &parent, int index);
        ~VectorAction() = default;

    public:
        inline int index() const;

    public:
        int _index;
    };

    inline int VectorAction::index() const {
        return _index;
    }

    inline VectorAction::VectorAction(Type type, const std::shared_ptr<Node> &parent, int index)
        : NodeAction(type, parent), _index(index) {
    }


    /// VectorMoveAction - Action for \c VectorNode movement.
    class SUBSTATE_EXPORT VectorMoveAction : public VectorAction {
    public:
        inline VectorMoveAction(const std::shared_ptr<Node> &parent, int index, int count,
                                int dest);
        ~VectorMoveAction() = default;

    public:
        void queryNodes(bool inserted,
                        const std::function<void(const std::shared_ptr<Node> &)> &add) override;
        void execute(bool undo) override;

    public:
        inline int count() const;
        inline int destination() const;

    protected:
        int _count, _dest;
    };

    inline VectorMoveAction::VectorMoveAction(const std::shared_ptr<Node> &parent, int index,
                                              int count, int dest)
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
        inline VectorInsDelAction(Type type, const std::shared_ptr<Node> &parent, int index,
                                  std::vector<std::shared_ptr<Node>> children);
        ~VectorInsDelAction() = default;

    public:
        void queryNodes(bool inserted,
                        const std::function<void(const std::shared_ptr<Node> &)> &add) override;
        void execute(bool undo) override;

    public:
        inline ArrayView<std::shared_ptr<Node>> children() const;

    protected:
        std::vector<std::shared_ptr<Node>> _children;
    };

    inline VectorInsDelAction::VectorInsDelAction(Type type, const std::shared_ptr<Node> &parent,
                                                  int index,
                                                  std::vector<std::shared_ptr<Node>> children)
        : VectorAction(type, parent, index), _children(std::move(children)) {
    }

    inline ArrayView<std::shared_ptr<Node>> VectorInsDelAction::children() const {
        return _children;
    }

}

#endif // SUBSTATE_VECTORNODE_H