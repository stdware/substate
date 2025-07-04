#include "SheetNode.h"
#include "SheetNode_p.h"

#include <cassert>
#include <utility>

#include "Model_p.h"
#include "Node_p.h"

namespace ss {

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
        dest->_maxId = src->_maxId;
    }

    SheetNode::~SheetNode() = default;

    int SheetNode::insert(const std::shared_ptr<Node> &node) {
        assert(isWritable());
        assert(node && node->isFree());

        int id = _maxId = _maxId + 1;
        auto a = std::make_unique<SheetAction>(
            Action::SheetInsert, std::static_pointer_cast<SheetNode>(shared_from_this()), id, node);
        a->execute(false);
        ModelPrivate::pushAction(_model, std::move(a));
        return id;
    }

    bool SheetNode::remove(int id) {
        assert(isWritable());

        auto it = _sheet.find(_id);
        if (it == _sheet.end()) {
            return false;
        }
        const auto &node = it->second;

        beginAction();

        auto a = std::make_unique<SheetAction>(
            Action::SheetRemove, std::static_pointer_cast<SheetNode>(shared_from_this()), id, node);

        // Pre-Propagate
        {
            ActionNotification n(Notification::ActionAboutToTrigger, a.get());
            notify(&n);
        }

        // Do change
        removeChild(node.get());
        _sheet.erase(it);

        // Propagate signal
        {
            ActionNotification n(Notification::ActionTriggered, a.get());
            notify(&n);
        }

        endAction();
        ModelPrivate::pushAction(_model, std::move(a));
        return true;
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

    void SheetAction::queryNodes(bool inserted,
                                 const std::function<void(const std::shared_ptr<Node> &)> &add) {
        if (inserted == (_type == Action::SheetInsert)) {
            add(_child);
        }
    }

    void SheetAction::execute(bool undo) {
        auto parent = static_cast<SheetNode *>(_parent.get());

        // Pre-Propagate
        {
            ActionNotification n(Notification::ActionAboutToTrigger, this);
            parent->notify(&n);
        }

        // Do change
        if ((_type == SheetRemove) ^ undo) {
            auto it = parent->_sheet.find(_id);
            assert(it != parent->_sheet.end());

            const auto &node = it->second;
            assert(node.get() == _child.get());

            // Do change
            parent->removeChild(node.get());
            parent->_sheet.erase(it);
        } else {
            parent->addChild(_child.get());
            parent->_sheet.insert(std::make_pair(_id, _child));
        }

        // Propagate signal
        {
            ActionNotification n(Notification::ActionTriggered, this);
            parent->notify(&n);
        }

        parent->endAction();
    }

}