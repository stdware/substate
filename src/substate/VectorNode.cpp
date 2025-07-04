#include "VectorNode.h"
#include "VectorNode_p.h"

#include <cassert>
#include <algorithm>
#include <utility>

#include "Model_p.h"
#include "Node_p.h"

namespace ss {

    template <class T>
    static inline void arrayMove(std::vector<T> &arr, int index, int count, int dest) {
        assert(dest != index && count > 0);
        if (dest < index) {
            std::rotate(arr.begin() + dest, arr.begin() + index, arr.begin() + index + count);
        } else {
            std::rotate(arr.begin() + index, arr.begin() + index + count,
                        arr.begin() + dest + count);
        }
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

    void VectorNode::insert(int index, std::vector<std::shared_ptr<Node>> nodes) {
        assert(isWritable());
        assert(NodePrivate::validateArrayQueryArguments(index, _vec.size()));
        assert(!nodes.empty());

#ifndef NDEBUG
        for (const auto &node : nodes) {
            assert(node && node->isFree());
        }
#endif

        auto action = std::make_unique<VectorInsDelAction>(
            Action::VectorInsert, std::static_pointer_cast<VectorNode>(shared_from_this()), index,
            std::move(nodes));
        action->execute(false);
        ModelPrivate::pushAction(_model, std::move(action));
    }

    void VectorNode::move(int index, int count, int dest) {
        assert(isWritable());
        assert(NodePrivate::validateArrayRemoveArguments(index, count, _vec.size()) &&
               !(dest >= index && dest < index + count));

        auto action = std::make_unique<VectorMoveAction>(
            std::static_pointer_cast<VectorNode>(shared_from_this()), index, count, dest);
        action->execute(false);
        ModelPrivate::pushAction(_model, std::move(action));
    }

    void VectorNode::remove(int index, int count) {
        assert(isWritable());
        assert(NodePrivate::validateArrayRemoveArguments(index, count, _vec.size()));

        std::vector<std::shared_ptr<Node>> nodes;
        nodes.resize(count);
        std::copy(_vec.begin() + index, _vec.begin() + index + count, nodes.begin());

        auto action = std::make_unique<VectorInsDelAction>(
            Action::VectorRemove, std::static_pointer_cast<VectorNode>(shared_from_this()), index,
            std::move(nodes));
        action->execute(false);
        ModelPrivate::pushAction(_model, std::move(action));
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

    void VectorMoveAction::queryNodes(
        bool inserted, const std::function<void(const std::shared_ptr<Node> &)> &add) {
        (void) inserted;
        (void) add;
    }

    void VectorMoveAction::execute(bool undo) {
        auto parent = static_cast<VectorNode *>(_parent.get());
        auto &vec = parent->_vec;

        parent->beginAction();
        // Pre-Propagate signal
        {
            ActionNotification n(Notification::ActionAboutToTrigger, this);
            parent->notify(&n);
        }

        // Do change
        int index;
        int dest;
        if (undo) {
            if (_dest > _index) {
                index = _dest - _count;
                dest = _index;
            } else {
                index = _dest;
                dest = _index + _count;
            }
        } else {
            index = _index;
            dest = _dest;
        }
        arrayMove(vec, index, _count, dest);

        // Propagate signal
        {
            ActionNotification n(Notification::ActionTriggered, this);
            parent->notify(&n);
        }
        parent->endAction();
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
        auto &vec = parent->_vec;

        parent->beginAction();
        // Pre-Propagate signal
        {
            ActionNotification n(Notification::ActionAboutToTrigger, this);
            parent->notify(&n);
        }

        // Do change
        if (((_type == VectorRemove) ^ undo)) {
            auto begin = vec.begin() + _index;
            auto end = vec.begin() + _index + _children.size();
            for (auto it = begin; it != end; ++it) {
                parent->removeChild(it->get());
            }
            vec.erase(begin, end);
        } else {
            for (const auto &node : std::as_const(_children)) {
                parent->addChild(node.get());
            }
            vec.insert(vec.begin() + _index, _children.begin(), _children.end());
        }

        // Post-propagate signal
        {
            ActionNotification n(Notification::ActionTriggered, this);
            parent->notify(&n);
        }
        parent->endAction();
    }

}