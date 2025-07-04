#include "MappingNode.h"
#include "MappingNode_p.h"

#include <cassert>

#include <substate/private/Model_p.h>
#include <substate/private/Node_p.h>

namespace ss {

    void MappingNodePrivate::copy(MappingNode *dest, const MappingNode *src, bool copyId) {
        if (copyId) {
            dest->_id = src->_id;
        }
        // Clone children
        for (auto &pair : src->_map) {
            const auto &key = pair.first;
            const auto &prop = pair.second;

            if (prop.isVariant()) {
                dest->_map.insert(std::make_pair(key, prop.variant()));
                continue;
            }

            auto newChild = NodePrivate::clone(prop.node().get(), copyId);
            dest->addChild(newChild.get());
            dest->_map.insert(std::make_pair(key, newChild));
        }
    }

    MappingNode::~MappingNode() = default;

    bool MappingNode::setProperty(const QString &key, const Property &value) {
        assert(isWritable());

        Property oldProp;
        auto it = _map.find(key);
        if (it == _map.end()) {
            // Nothing changes
            if (!value.isValid())
                return false;
        } else {
            // Nothing changes
            if (value == it->second)
                return false;
            oldProp = it->second;
        }

        beginAction();

        auto a = std::make_unique<MappingAction>(
            std::static_pointer_cast<MappingNode>(shared_from_this()), key, oldProp, value);

        // Pre-Propagate signal
        {
            ActionNotification n(Notification::ActionAboutToTrigger, a.get());
            notify(&n);
        }

        // Do change
        if (it == _map.end()) {
            _map.insert(std::make_pair(key, value));
        } else {
            if (value.isValid()) {
                it->second = value;
            } else {
                _map.erase(it);
            }
        }

        if (oldProp.isNode()) {
            auto oldNode = oldProp.node();
            removeChild(oldNode.get());
        }
        if (value.node()) {
            auto node = value.node();
            addChild(node.get());
        }

        // Propagate signal
        {
            ActionNotification n(Notification::ActionTriggered, a.get());
            notify(&n);
        }

        // Push
        endAction();
        ModelPrivate::pushAction(_model, std::move(a));
        return true;
    }

    std::shared_ptr<Node> MappingNode::clone(bool copyId) const {
        auto node = std::make_shared<MappingNode>(_type);
        MappingNodePrivate::copy(node.get(), this, copyId);
        return node;
    }

    void MappingNode::propagateChildren(const std::function<void(Node *)> &func) {
        for (const auto &pair : std::as_const(_map)) {
            const auto &prop = pair.second;
            if (prop.isNode()) {
                NodePrivate::propagate(prop.node().get(), func);
            }
        }
    }

    MappingAction::~MappingAction() = default;

    void MappingAction::execute(bool undo) {
        auto parent = static_cast<MappingNode *>(_parent.get());

        auto &key = _key;
        auto &value = undo ? _oldValue : _value;
        auto &map = parent->_map;
        Property oldProp;

        parent->beginAction();

        auto it = map.find(key);
        if (it == map.end()) {
            assert(value.isValid());
        } else {
            assert(value != it->second);
            oldProp = it->second;
        }

        MappingAction a(std::static_pointer_cast<MappingNode>(parent->shared_from_this()), key,
                        oldProp, value);

        // Pre-Propagate signal
        {
            ActionNotification n(Notification::ActionAboutToTrigger, &a);
            parent->notify(&n);
        }

        // Do change
        if (it == map.end()) {
            map.insert(std::make_pair(key, value));
        } else {
            if (value.isValid()) {
                it->second = value;
            } else {
                map.erase(it);
            }
        }

        if (oldProp.isNode()) {
            auto oldNode = oldProp.node();
            parent->removeChild(oldNode.get());
        }
        if (value.node()) {
            auto node = value.node();
            parent->addChild(node.get());
        }

        // Propagate signal
        {
            ActionNotification n(Notification::ActionTriggered, &a);
            parent->notify(&n);
        }

        parent->endAction();
    }

}