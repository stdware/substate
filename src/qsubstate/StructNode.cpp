#include "StructNode.h"
#include "StructNode_p.h"

#include <substate/private/Node_p.h>
#include <substate/private/Model_p.h>

namespace ss {

    void StructNodeBasePrivate::copy(StructNodeBase *dest, const StructNodeBase *src, bool copyId) {
        if (copyId) {
            dest->_id = src->_id;
        }
        // Clone children
        auto size = src->_size;
        for (size_t i = 0; i < size; ++i) {
            const auto &prop = src->_storage[i];

            if (prop.isVariant()) {
                dest->_storage[i] = prop;
                continue;
            }

            auto newChild = NodePrivate::clone(prop.node().get(), copyId);
            dest->addChild(newChild.get());
            dest->_storage[i] = newChild;
        }
    }

    StructNodeBase::~StructNodeBase() = default;

    void StructNodeBase::setAt(int index, Property value) {
        assert(isWritable());
        assert(index >= 0 && index < _storage.size());

        auto action = std::make_unique<StructAction>(
            std::static_pointer_cast<StructNodeBase>(shared_from_this()), index, _storage[index],
            std::move(value));
        action->execute(false);
        ModelPrivate::pushAction(_model, std::move(action));
    }

    void StructNodeBase::propagateChildren(const std::function<void(Node *)> &func) {
        for (size_t i = 0; i < _size; ++i) {
            const auto &prop = _storage[i];
            if (prop.isNode()) {
                NodePrivate::propagate(prop.node().get(), func);
            }
        }
    }

    void StructNodeBase::copy(StructNodeBase *dest, const StructNodeBase *src, bool copyId) {
        StructNodeBasePrivate::copy(dest, src, copyId);
    }

    StructAction::~StructAction() = default;

    void StructAction::execute(bool undo) {
        auto parent = static_cast<StructNodeBase *>(_parent.get());

        auto &index = _index;
        auto &value = undo ? _oldValue : _value;
        auto &storage = parent->_storage;
        Property oldProp = storage[index];

        parent->beginAction();

        StructAction a(std::static_pointer_cast<StructNodeBase>(parent->shared_from_this()), index,
                       oldProp, value);

        // Pre-Propagate signal
        {
            ActionNotification n(Notification::ActionAboutToTrigger, &a);
            parent->notify(&n);
        }

        // Do change
        storage[index] = value;

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