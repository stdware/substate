#include "StructNode.h"

#include <cassert>

#include <substate/private/Node_p.h>

#include "Property_p.h"

namespace ss {

    StructNodeBase::~StructNodeBase() = default;

    void StructNodeBase::setAt(int index, Property value) {
        assert(isWritable());
        assert(index >= 0 && index < m_size);
        assert(PropertyPrivate::isAssignable(value, this));

        auto &slot = m_slots[index];
        if (slot == value) {
            return;
        }

        if (isFree()) {
            PropertyPrivate::assignFree(this, slot, std::move(value));
            return;
        }

        std::unique_ptr<StructAssignAction> action(
            new StructAssignAction(this, index, std::move(value)));
        action->execute(Action::Execute);
        NodePrivate::pushAction(model(), std::move(action));
    }

    Property StructNodeBase::take(int index) {
        assert(isFree());
        assert(index >= 0 && index < m_size);
        return PropertyPrivate::take(m_slots[index]);
    }

    void StructNodeBase::forEachChild(const std::function<void(Node *)> &func) const {
        for (int i = 0; i < m_size; ++i) {
            if (auto child = m_slots[i].child()) {
                func(child);
            }
        }
    }

    void StructNodeBase::cloneSlotsFrom(const StructNodeBase &source) {
        assert(isFree() && m_size == source.m_size);
        for (int i = 0; i < m_size; ++i) {
            assert(m_slots[i].isEmpty());
            PropertyPrivate::assignFree(this, m_slots[i], source.m_slots[i].clone());
        }
    }

    StructAssignAction::StructAssignAction(StructNodeBase *parent, int index, Property value)
        : PropertyAction(StructAssign, parent, parent->at(index), std::move(value)),
          m_index(index) {
    }

    StructAssignAction::~StructAssignAction() = default;

    void StructAssignAction::execute(Operation operation) {
        (void) operation;
        exchange(static_cast<StructNodeBase *>(parent())->m_slots[m_index]);
    }

}
