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

    void VectorNode::insert(int index, std::vector<NodePtr> nodes) {
        assert(isWritable());
        assert(NodePrivate::validateArrayQueryArguments(index, _vec.size()));
        assert(!nodes.empty());

#ifndef NDEBUG
        for (const auto &node : nodes) {
            assert(node && node->isFree());
        }
#endif

        auto action = std::make_unique<VectorInsDelAction>(Action::VectorInsert, this, index,
                                                           std::move(nodes));
        action->execute(false);
        ModelPrivate::pushAction(_model, std::move(action));
    }

    void VectorNode::move(int index, int count, int dest) {
        assert(isWritable());
        assert(NodePrivate::validateArrayRemoveArguments(index, count, _vec.size()) &&
               !(dest >= index && dest < index + count));

        auto action = std::make_unique<VectorMoveAction>(this, index, count, dest);
        action->execute(false);
        ModelPrivate::pushAction(_model, std::move(action));
    }

    void VectorNode::remove(int index, int count) {
        assert(isWritable());
        assert(NodePrivate::validateArrayRemoveArguments(index, count, _vec.size()));

        std::vector<NodePtr> nodes;
        nodes.resize(count);
        for (size_t i = 0; i < count; ++i) {
            nodes[i] = _vec[index + i].makeRef();
        }
        auto action = std::make_unique<VectorInsDelAction>(Action::VectorRemove, this, index,
                                                           std::move(nodes));
        action->execute(false);
        ModelPrivate::pushAction(_model, std::move(action));
    }

    NodePtr VectorNode::clone(bool copyId) const {
        auto node = makeSmart<VectorNode>(_type);
        VectorNodePrivate::copy(node.get(), this, copyId);
        return node;
    }

    void VectorNode::propagateChildren(const std::function<void(Node *)> &func) {
        for (const auto &node : std::as_const(_vec)) {
            NodePrivate::propagate(node.get(), func);
        }
    }

    void VectorMoveAction::queryNodes(bool inserted,
                                      const std::function<void(const NodePtr &)> &add) {
        (void) inserted;
        (void) add;
    }

    void VectorMoveAction::execute(bool undo) {
        auto parent = static_cast<VectorNode *>(_parent.get());
        auto &vec = parent->_vec;

        parent->beginAction();
        // Pre-Propagate signal
        {
            if (undo) {
                // TODO
            }

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
            if (undo) {
                // TODO
            }

            ActionNotification n(Notification::ActionTriggered, this);
            parent->notify(&n);
        }
        parent->endAction();
    }

    void VectorInsDelAction::queryNodes(bool inserted,
                                        const std::function<void(const NodePtr &)> &add) {
        if (inserted == (_type == VectorInsert)) {
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
            auto orgType = _type;
            if (undo) {
                _type = _type == VectorInsert ? VectorRemove : VectorInsert;
            }

            ActionNotification n(Notification::ActionAboutToTrigger, this);
            parent->notify(&n);

            _type = orgType;
        }

        // Do change
        if (((_type == VectorRemove) ^ undo)) {
            for (size_t i = 0; i < _children.size(); ++i) {
                auto &orgNode = vec[_index + i];
                _children[i].swap(orgNode);
                parent->removeChild(orgNode.get());
            }
            vec.erase(vec.begin() + _index, vec.begin() + _index + _children.size());
        } else {
            for (const auto &node : std::as_const(_children)) {
                parent->addChild(node.get());
            }
            vec.insert(vec.begin() + _index, _children.size(), NodePtr());
            for (size_t i = 0; i < _children.size(); ++i) {
                auto &node = _children[i];
                auto &newNode = vec[_index + i];
                newNode = node.makeRef();
                newNode.swap(node);
            }
        }

        // Post-propagate signal
        {
            auto orgType = _type;
            if (undo) {
                _type = _type == VectorInsert ? VectorRemove : VectorInsert;
            }

            ActionNotification n(Notification::ActionTriggered, this);
            parent->notify(&n);

            _type = orgType;
        }
        parent->endAction();
    }

}
