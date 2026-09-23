#include "Property.h"
#include "Property_p.h"

#include <cassert>
#include <utility>

#include <substate/private/Node_p.h>

namespace ss {

    Property::~Property() = default;

    Property::Property(Property &&RHS) noexcept = default;

    Property &Property::operator=(Property &&RHS) noexcept = default;

    Property Property::clone() const {
        switch (type()) {
            case Variant:
                return Property(variant());
            case Child:
                return Property(child()->clone());
            default:
                break;
        }
        return {};
    }

    bool Property::operator==(const Property &RHS) const {
        if (type() != RHS.type()) {
            return false;
        }
        switch (type()) {
            case Variant:
                return variant() == RHS.variant();
            case Child:
                return child() == RHS.child();
            default:
                break;
        }
        return true;
    }

    bool PropertyPrivate::isAssignable(const Property &value, const Node *parent) {
        auto child = value.child();
        return !child ||
               (NodePrivate::isInsertable(child) && !NodePrivate::isAncestorOrSelf(child, parent));
    }

    void PropertyPrivate::assignFree(Node *parent, Property &slot, Property value) {
        assert(parent->isFree());
        if (auto child = value.child()) {
            NodePrivate::setFreeParent(child, parent);
        }
        slot = std::move(value);
    }

    Property PropertyPrivate::take(Property &slot) {
        Property value = std::move(slot);
        slot = Property();
        if (auto child = value.child()) {
            NodePrivate::setFreeParent(child, nullptr);
        }
        return value;
    }

    void PropertyPrivate::exchange(Node *parent, Property &slot, Property &other) {
        std::swap(slot.m_value, other.m_value);
        if (auto leaving = other.child()) {
            NodePrivate::detach(leaving);
        }
        if (auto entering = slot.child()) {
            NodePrivate::attach(entering, parent, parent->model());
        }
    }

    std::unique_ptr<Node> PropertyPrivate::releaseChild(Property &slot) {
        auto child = std::get_if<std::unique_ptr<Node>>(&slot.m_value);
        assert(child);
        std::unique_ptr<Node> node = std::move(*child);
        slot.m_value = std::monostate();
        return node;
    }

    PropertyAction::PropertyAction(int type, Node *parent, const Property &oldValue,
                                   Property newValue)
        : Action(type), m_parent(parent), m_newVariant(newValue.variant()),
          m_newChild(newValue.child()), m_oldVariant(oldValue.variant()),
          m_oldChild(oldValue.child()), m_held(std::move(newValue)) {
    }

    PropertyAction::~PropertyAction() = default;

    void PropertyAction::forEachHeldNode(const std::function<void(Node *)> &func) const {
        if (auto child = m_held.child()) {
            func(child);
        }
    }

    void PropertyAction::exchange(Property &slot) {
        PropertyPrivate::exchange(m_parent, slot, m_held);
    }

}
