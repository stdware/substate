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

        // Reads a value written by writeParts() into variant, or reads the child with readChild,
        // which returns whether it read one. Records a failure if the value is invalid.
        template <class ReadChild>
        void readParts(Decoder &decoder, QVariant &variant, ReadChild readChild) {
            uint8_t tag = 0;
            decoder.stream() >> tag;
            if (decoder.fail()) {
                return;
            }
            bool valid = true;
            switch (tag) {
                case EmptyTag:
                    break;
                case VariantTag:
                    variant = QCodec::readVariant(decoder);
                    valid = variant.isValid();
                    break;
                case ChildTag:
                    valid = readChild();
                    break;
                default:
                    valid = false;
                    break;
            }
            if (!valid) {
                decoder.setFailed();
                variant = QVariant();
            }
        }

    }

    void PropertyPrivate::write(Encoder &encoder, const Property &value) {
        writeParts(encoder, value.variant(), value.child(), false);
    }

    Property PropertyPrivate::read(Decoder &decoder) {
        QVariant variant;
        std::unique_ptr<Node> child;
        readParts(decoder, variant, [&] {
            child = decoder.readNode();
            return bool(child);
        });
        if (decoder.fail()) {
            return {};
        }
        return child ? Property(std::move(child)) : Property(std::move(variant));
    }

    PropertyAction::PropertyAction(int type, Node *parent, const Property &oldValue,
                                   Property newValue)
        : Action(type), m_parent(parent), m_newVariant(newValue.variant()),
          m_newChild(newValue.child()), m_oldVariant(oldValue.variant()),
          m_oldChild(oldValue.child()), m_held(std::move(newValue)) {
    }

    PropertyAction::PropertyAction(int type, Node *parent, DecodedValues values)
        : Action(type), m_parent(parent), m_newVariant(std::move(values.newVariant)),
          m_newChild(values.newChild), m_oldVariant(std::move(values.oldVariant)),
          m_oldChild(values.oldChild), m_held(std::move(values.held)) {
    }

    PropertyAction::~PropertyAction() = default;

    void PropertyAction::writeValues(Encoder &encoder) const {
        // The new value is owned by the action while unapplied, the old value while applied.
        // Only the first is written with its content, see Encoder::writeAction().
        writeParts(encoder, m_newVariant, m_newChild, false);
        writeParts(encoder, m_oldVariant, m_oldChild, true);
    }

    PropertyAction::DecodedValues PropertyAction::readValues(Decoder &decoder, State state) {
        DecodedValues values;
        std::unique_ptr<Node> owned;
        if (state == Unapplied) {
            readParts(decoder, values.newVariant, [&] {
                owned = decoder.readNode();
                values.newChild = owned.get();
                return bool(owned);
            });
            readParts(decoder, values.oldVariant, [&] {
                values.oldChild = decoder.readReference();
                return values.oldChild != nullptr;
            });
            values.held = owned ? Property(std::move(owned)) : Property(values.newVariant);
        } else {
            readParts(decoder, values.newVariant, [&] {
                values.newChild = decoder.readExistingNode();
                return values.newChild != nullptr;
            });
            readParts(decoder, values.oldVariant, [&] {
                owned = decoder.takeReference();
                values.oldChild = owned.get();
                return bool(owned);
            });
            values.held = owned ? Property(std::move(owned)) : Property(values.oldVariant);
        }
        return values;
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
