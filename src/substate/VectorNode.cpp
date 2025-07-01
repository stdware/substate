#include "VectorNode.h"
#include "VectorNode_p.h"

#include <cassert>
#include <algorithm>
#include <utility>

#include "Node_p.h"

namespace ss {

    // Move item inside the array
    // TODO: use std::rotate
    template <class T>
    static void arrayMove(std::vector<T> &arr, int index, int count, int dest) {
        std::vector<T> tmp;
        tmp.resize(count);
        std::copy(arr.begin() + index, arr.begin() + index + count, tmp.begin());

        // Do change
        int correctDest;
        if (dest > index) {
            correctDest = dest - count;
            auto sz = correctDest - index;
            for (int i = 0; i < sz; ++i) {
                arr[index + i] = arr[index + count + i];
            }
        } else {
            correctDest = dest;
            auto sz = index - dest;
            for (int i = sz - 1; i >= 0; --i) {
                arr[dest + count + i] = arr[dest + i];
            }
        }
        std::copy(tmp.begin(), tmp.end(), arr.begin() + correctDest);

        // TODO: Using std::rotate
        // if (dest < index) {
        //     // 目标在源之前：旋转区间[dest, index+count) 使块移动到dest
        //     std::rotate(arr.begin() + dest, arr.begin() + index, arr.begin() + index + count);
        // } else { // dest > index
        //     // 目标在源之后：旋转区间[index, dest+count) 使块移动到dest
        //     std::rotate(arr.begin() + index, arr.begin() + index + count,
        //                 arr.begin() + dest + count);
        // }
    }

    void VectorNodePrivate::insert_TX(VectorNode *q, int index,
                                      const ArrayView<std::shared_ptr<Node>> &nodes) {
        q->beginAction();

        auto insertedVec = nodes.vec();
        auto &vec = q->_vec;

        VectorInsDelAction a(Action::VectorInsert, q->shared_from_this(), index, nodes.vec());

        // Pre-Propagate
        {
            ActionNotification n(Notification::ActionAboutToTrigger, &a);
            q->notified(&n);
        }

        // Do change
        for (const auto &node : std::as_const(insertedVec)) {
            q->addChild(node.get());
        }
        vec.insert(vec.begin() + index, insertedVec.begin(), insertedVec.end());

        // Propagate signal
        {
            ActionNotification n(Notification::ActionTriggered, &a);
            q->notified(&n);
        }

        q->endAction();
    }

    void VectorNodePrivate::remove_TX(VectorNode *q, int index, int count) {
        q->beginAction();

        auto &vec = q->_vec;

        std::vector<std::shared_ptr<Node>> removedVec;
        removedVec.resize(count);
        std::copy(vec.begin() + index, vec.begin() + index + count, removedVec.begin());

        VectorInsDelAction a(Action::VectorRemove, q->shared_from_this(), index, removedVec);

        // Pre-Propagate signal
        {
            ActionNotification n(Notification::ActionAboutToTrigger, &a);
            q->notified(&n);
        }

        // Do change
        vec.erase(vec.begin() + index, vec.begin() + index + count);
        for (const auto &node : std::as_const(removedVec)) {
            q->removeChild(node.get());
        }

        // Propagate signal
        {
            ActionNotification n(Notification::ActionTriggered, &a);
            q->notified(&n);
        }

        q->endAction();
    }

    void VectorNodePrivate::move_TX(VectorNode *q, int index, int count, int dest) {
        q->beginAction();

        auto &vec = q->_vec;

        VectorMoveAction a(q->shared_from_this(), index, count, dest);

        // Pre-Propagate signal
        {
            ActionNotification n(Notification::ActionAboutToTrigger, &a);
            q->notified(&n);
        }

        // Do change
        arrayMove(vec, index, count, dest);

        // Propagate signal
        {
            ActionNotification n(Notification::ActionTriggered, &a);
            q->notified(&n);
        }

        q->endAction();
    }

    void VectorNodePrivate::copy(VectorNode *dest, const VectorNode *src, bool copyId) {
        if (!copyId) {
            dest->_id = src->_id;
        }
        // Clone children
        dest->_vec.reserve(src->_vec.size());
        for (auto &child : src->_vec) {
            auto newChild = NodePrivate::clone(child.get(), copyId);
            dest->addChild(newChild.get());
            dest->_vec.emplace_back(std::move(newChild));
        }
    }

    VectorNode::~VectorNode() = default;

    void VectorNode::insert(int index, const ArrayView<std::shared_ptr<Node>> &nodes) {
        assert(isWritable());
        assert(VectorNodePrivate::validateArrayQueryArguments(index, _vec.size()));
        assert(!nodes.empty());

#ifndef NDEBUG
        for (const auto &node : nodes) {
            assert(node && node->isFree());
        }
#endif

        VectorNodePrivate::insert_TX(this, index, nodes);
    }

    void VectorNode::move(int index, int count, int dest) {
        assert(isWritable());
        assert(VectorNodePrivate::validateArrayRemoveArguments(index, count, _vec.size()) &&
               !(dest >= index && dest < index + count));
        VectorNodePrivate::move_TX(this, index, count, dest);
    }

    void VectorNode::remove(int index, int count) {
        assert(isWritable());
        assert(VectorNodePrivate::validateArrayRemoveArguments(index, count, _vec.size()));
        VectorNodePrivate::remove_TX(this, index, count);
    }

    std::shared_ptr<Node> VectorNode::clone(bool copyId) const {
        auto node = std::make_shared<VectorNode>(_type);
        VectorNodePrivate::copy(node.get(), this, copyId);
        return node;
    }

    void VectorNode::propagateChildren(const std::function<void(Node *)> &func) {
        for (const auto &node : std::as_const(_vec)) {
            NodePrivate::propagate(node.get(), func);
        }
    }

    std::unique_ptr<Action> VectorMoveAction::clone(bool detach) const {
        return std::make_unique<VectorMoveAction>(_parent, _index, _count, _dest);
    }

    void VectorMoveAction::queryNodes(
        bool inserted, const std::function<void(const std::shared_ptr<Node> &)> &add) {
        (void) inserted;
        (void) add;
    }

    void VectorMoveAction::execute(bool undo) {
        auto parent = static_cast<VectorNode *>(_parent.get());
        if (undo) {
            int r_index;
            int r_dest;
            if (_dest > _index) {
                r_index = _dest - _count;
                r_dest = _index;
            } else {
                r_index = _dest;
                r_dest = _index + _count;
            }
            VectorNodePrivate::move_TX(parent, r_index, _count, r_dest);
        } else {
            VectorNodePrivate::move_TX(parent, _index, _count, _dest);
        }
    }

    std::unique_ptr<Action> VectorInsDelAction::clone(bool detach) const {
        if (detach) {
            std::vector<std::shared_ptr<Node>> children;
            children.reserve(_children.size());
            for (const auto &node : std::as_const(_children)) {
                children.emplace_back(NodePrivate::clone(node.get(), true));
            }
            return std::make_unique<VectorInsDelAction>(static_cast<Type>(_type), _parent, _index,
                                                        std::move(children));
        }
        return std::make_unique<VectorInsDelAction>(static_cast<Type>(_type), _parent, _index,
                                                    _children);
    }

    void VectorInsDelAction::queryNodes(
        bool inserted, const std::function<void(const std::shared_ptr<Node> &)> &add) {
        if (inserted == (_type == Action::VectorInsert)) {
            for (const auto &node : std::as_const(_children)) {
                add(node);
            }
        }
    }

    void VectorInsDelAction::execute(bool undo) {
        auto parent = static_cast<VectorNode *>(_parent.get());
        ((_type == VectorRemove) ^ undo)
            ? VectorNodePrivate::remove_TX(parent, _index, _children.size())
            : VectorNodePrivate::insert_TX(parent, _index, _children);
    }

}