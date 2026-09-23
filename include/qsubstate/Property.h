// Copyright (C) 2022-2025 Stdware Collections (https://www.github.com/stdware)
// SPDX-License-Identifier: Apache-2.0

#ifndef QSUBSTATE_PROPERTY_H
#define QSUBSTATE_PROPERTY_H

#include <memory>
#include <type_traits>
#include <variant>

#include <QtCore/QVariant>

#include <substate/Action.h>
#include <substate/Node.h>

#include <qsubstate/qsubstate_global.h>

namespace ss {

    class PropertyPrivate;

    /// The content of a slot of a StructNode or of an entry of a MappingNode: empty, a scalar
    /// value, or a child node.
    ///
    /// A Property that holds a child owns it, and can therefore only be moved. clone() copies the
    /// child. An invalid QVariant is stored as an empty Property.
    class QSUBSTATE_EXPORT Property {
    public:
        enum Type {
            Empty,
            Variant,
            Child,
        };

        inline Property();
        inline Property(QVariant variant);
        inline Property(std::unique_ptr<Node> child);

        template <class T, class = std::enable_if_t<std::is_base_of_v<Node, T>>>
        inline Property(std::unique_ptr<T> child);

        ~Property();

        Property(Property &&RHS) noexcept;
        Property &operator=(Property &&RHS) noexcept;

        inline Type type() const;
        inline bool isEmpty() const;
        inline bool isVariant() const;
        inline bool isChild() const;

        /// The scalar value, or an invalid QVariant if the Property does not hold one.
        inline QVariant variant() const;

        /// The child, or \c nullptr if the Property does not hold one.
        inline Node *child() const;

        /// Returns a copy, with a copy of the child as a free node without identifiers.
        Property clone() const;

        /// Returns whether both are empty, hold equal scalar values, or hold the same child.
        bool operator==(const Property &RHS) const;
        inline bool operator!=(const Property &RHS) const;

    private:
        std::variant<std::monostate, QVariant, std::unique_ptr<Node>> m_value;

        friend class PropertyPrivate;
    };

    inline Property::Property() = default;

    inline Property::Property(QVariant variant) {
        if (variant.isValid()) {
            m_value = std::move(variant);
        }
    }

    inline Property::Property(std::unique_ptr<Node> child) {
        if (child) {
            m_value = std::move(child);
        }
    }

    template <class T, class>
    inline Property::Property(std::unique_ptr<T> child)
        : Property(std::unique_ptr<Node>(std::move(child))) {
    }

    inline Property::Type Property::type() const {
        return Type(m_value.index());
    }

    inline bool Property::isEmpty() const {
        return type() == Empty;
    }

    inline bool Property::isVariant() const {
        return type() == Variant;
    }

    inline bool Property::isChild() const {
        return type() == Child;
    }

    inline QVariant Property::variant() const {
        auto value = std::get_if<QVariant>(&m_value);
        return value ? *value : QVariant();
    }

    inline Node *Property::child() const {
        auto value = std::get_if<std::unique_ptr<Node>>(&m_value);
        return value ? value->get() : nullptr;
    }

    inline bool Property::operator!=(const Property &RHS) const {
        return !(*this == RHS);
    }

    /// Assignment of a Property, the base of StructAssignAction and MappingAssignAction.
    ///
    /// Owns the value that is not in the node: the new value before execution and after undo,
    /// the old value after execution. Every operation exchanges the two values.
    class QSUBSTATE_EXPORT PropertyAction : public Action {
    public:
        ~PropertyAction();

        inline Node *parent() const;

        /// The scalar value after the action is applied for \a operation, or an invalid QVariant
        /// if the value is not a scalar value.
        inline QVariant newVariant(Operation operation = Execute) const;

        /// The child after the action is applied for \a operation, or \c nullptr if the value is
        /// not a child.
        inline Node *newChild(Operation operation = Execute) const;

        /// The scalar value before the action is applied for \a operation, or an invalid QVariant
        /// if the value is not a scalar value.
        inline QVariant oldVariant(Operation operation = Execute) const;

        /// The child before the action is applied for \a operation, or \c nullptr if the value is
        /// not a child.
        inline Node *oldChild(Operation operation = Execute) const;

        void forEachHeldNode(const std::function<void(Node *)> &func) const override;

    protected:
        PropertyAction(int type, Node *parent, const Property &oldValue, Property newValue);

        /// Exchanges \a slot, the value in the node, with the value that this action owns, and
        /// updates the parent of the children involved.
        void exchange(Property &slot);

    private:
        Node *m_parent;
        QVariant m_newVariant;
        Node *m_newChild;
        QVariant m_oldVariant;
        Node *m_oldChild;
        Property m_held;
    };

    inline Node *PropertyAction::parent() const {
        return m_parent;
    }

    inline QVariant PropertyAction::newVariant(Operation operation) const {
        return isForward(operation) ? m_newVariant : m_oldVariant;
    }

    inline Node *PropertyAction::newChild(Operation operation) const {
        return isForward(operation) ? m_newChild : m_oldChild;
    }

    inline QVariant PropertyAction::oldVariant(Operation operation) const {
        return isForward(operation) ? m_oldVariant : m_newVariant;
    }

    inline Node *PropertyAction::oldChild(Operation operation) const {
        return isForward(operation) ? m_oldChild : m_newChild;
    }

}

#endif // QSUBSTATE_PROPERTY_H
