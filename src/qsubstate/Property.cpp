#include "Property.h"
#include "Property_p.h"

#include <cassert>
#include <cstdint>
#include <utility>

#include <substate/private/Node_p.h>

#include "QCodec.h"

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

    namespace {

        // The encodings of a Property.
        enum PropertyTag : uint8_t {
            EmptyTag,
            VariantTag,
            ChildTag,
        };

        // Writes a value given by its parts, with the child written with its content or as a
        // reference.
        void writeParts(Encoder &encoder, const QVariant &variant, const Node *child,
                        bool reference) {
            if (child) {
                encoder.stream() << uint8_t(ChildTag);
                if (reference) {
                    encoder.writeReference(child);
                } else {
                    encoder.writeNode(child);
                }
            } else if (variant.isValid()) {
                encoder.stream() << uint8_t(VariantTag);
                QCodec::writeVariant(encoder, variant);
            } else {
                encoder.stream() << uint8_t(EmptyTag);
            }
        }

    }

    void PropertyPrivate::write(Encoder &encoder, const Property &value) {
        writeParts(encoder, value.variant(), value.child(), false);
    }

    Property PropertyPrivate::read(Decoder &decoder) {
        uint8_t tag = 0;
        decoder.stream() >> tag;
        Property value;
        if (decoder.fail()) {
            return value;
        }
        switch (tag) {
            case EmptyTag:
                break;
            case VariantTag:
                value = QCodec::readVariant(decoder);
                break;
            case ChildTag:
                value = decoder.readNode();
                break;
            default:
                break;
        }
        if (decoder.fail() || (tag != EmptyTag && value.isEmpty())) {
            decoder.setFailed();
            return {};
        }
        return value;
    }

    void PropertyPrivate::writeReference(Encoder &encoder, const QVariant &variant,
                                         const Node *child) {
        writeParts(encoder, variant, child, true);
    }

    std::pair<QVariant, Node *> PropertyPrivate::readReference(Decoder &decoder) {
        uint8_t tag = 0;
        decoder.stream() >> tag;
        std::pair<QVariant, Node *> value{QVariant(), nullptr};
        if (decoder.fail()) {
            return value;
        }
        switch (tag) {
            case EmptyTag:
                break;
            case VariantTag:
                value.first = QCodec::readVariant(decoder);
                break;
            case ChildTag:
                value.second = decoder.readReference();
                break;
            default:
                break;
        }
        if (decoder.fail() || (tag != EmptyTag && !value.first.isValid() && !value.second)) {
            decoder.setFailed();
            return {QVariant(), nullptr};
        }
        return value;
    }

    PropertyAction::PropertyAction(int type, Node *parent, const Property &oldValue,
                                   Property newValue)
        : PropertyAction(type, parent, oldValue.variant(), oldValue.child(), std::move(newValue)) {
    }

    PropertyAction::PropertyAction(int type, Node *parent, QVariant oldVariant, Node *oldChild,
                                   Property newValue)
        : Action(type), m_parent(parent), m_newVariant(newValue.variant()),
          m_newChild(newValue.child()), m_oldVariant(std::move(oldVariant)), m_oldChild(oldChild),
          m_held(std::move(newValue)) {
    }

    PropertyAction::~PropertyAction() = default;

    void PropertyAction::writeValues(Encoder &encoder) const {
        // The new value is owned by the action before its first execution, the old value is not.
        writeParts(encoder, m_newVariant, m_newChild, false);
        writeParts(encoder, m_oldVariant, m_oldChild, true);
    }

    void PropertyAction::forEachHeldNode(const std::function<void(Node *)> &func) const {
        if (auto child = m_held.child()) {
            func(child);
        }
    }

    void PropertyAction::exchange(Property &slot) {
        PropertyPrivate::exchange(m_parent, slot, m_held);
    }

}
