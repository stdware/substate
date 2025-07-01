#include "SheetNode.h"
#include "SheetNode_p.h"

#include <cassert>

#include "Node_p.h"

namespace ss {

    void SheetNodePrivate::insert_TX(SheetNode *q, int id, const std::shared_ptr<Node> &node) {
        q->beginAction();

        SheetAction a(Action::SheetInsert, q->shared_from_this(), id, node);

        // Pre-Propagate
        {
            ActionNotification n(Notification::ActionAboutToTrigger, &a);
            q->notified(&n);
        }

        // Do change
        q->addChild(node.get());

        q->_sheet.insert(std::make_pair(id, node));
        q->_idSet.insert(id);

        // Propagate signal
        {
            ActionNotification n(Notification::ActionTriggered, &a);
            q->notified(&n);
        }

        q->endAction();
    }

    bool SheetNodePrivate::remove_TX(SheetNode *q, int id) {
        q->beginAction();

        auto it = q->_sheet.find(id);
        if (it == q->_sheet.end())
            return false;

        const auto &node = it->second;

        SheetAction a(Action::SheetRemove, q->shared_from_this(), id, node);

        // Pre-Propagate signal
        {
            ActionNotification n(Notification::ActionAboutToTrigger, &a);
            q->notified(&n);
        }

        // Do change
        q->removeChild(node.get());
        q->_sheet.erase(it);
        q->_idSet.erase(id);

        // Propagate signal
        {
            ActionNotification n(Notification::ActionTriggered, &a);
            q->notified(&n);
        }

        q->endAction();
        return true;
    }

    void SheetNodePrivate::copy(SheetNode *dest, const SheetNode *src, bool copyId) {
        if (!copyId) {
            dest->_id = src->_id;
        }
        // Clone children
        dest->_sheet.reserve(src->_sheet.size());
        for (auto it = src->_sheet.begin(); it != src->_sheet.end(); ++it) {
            auto newChild = NodePrivate::clone(it->second.get(), copyId);
            dest->addChild(newChild.get());
            dest->_sheet.insert(std::make_pair(it->first, newChild));
        }
        dest->_idSet = src->_idSet;
    }

    SheetNode::~SheetNode() = default;

    int SheetNode::insert(const std::shared_ptr<Node> &node) {
        assert(isWritable());
        assert(node && node->isFree());

        auto id = _idSet.empty() ? 1 : (*_idSet.rbegin() + 1);
        SheetNodePrivate::insert_TX(this, id, node);
        return id;
    }

    bool SheetNode::remove(int id) {
        assert(isWritable());
        return SheetNodePrivate::remove_TX(this, id);
    }

    std::shared_ptr<Node> SheetNode::clone(bool copyId) const {
        auto node = std::make_shared<SheetNode>(_type);
        SheetNodePrivate::copy(node.get(), this, copyId);
        return node;
    }

    void SheetNode::propagateChildren(const std::function<void(Node *)> &func) {
        for (const auto &pair : std::as_const(_sheet)) {
            NodePrivate::propagate(pair.second.get(), func);
        }
    }

    std::unique_ptr<Action> SheetAction::clone(bool detach) const {
        if (detach) {
            return std::make_unique<SheetAction>(static_cast<Type>(_type), _parent, _id,
                                                 NodePrivate::clone(_child.get(), true));
        }
        return std::make_unique<SheetAction>(static_cast<Type>(_type), _parent, _id, _child);
    }

    void SheetAction::queryNodes(bool inserted,
                                 const std::function<void(const std::shared_ptr<Node> &)> &add) {
        if (inserted == (_type == Action::SheetInsert)) {
            add(_child);
        }
    }

    void SheetAction::execute(bool undo) {
        auto parent = static_cast<SheetNode *>(_parent.get());
        ((_type == SheetRemove) ^ undo) ? (void) SheetNodePrivate::remove_TX(parent, _id)
                                        : SheetNodePrivate::insert_TX(parent, _id, _child);
    }

}