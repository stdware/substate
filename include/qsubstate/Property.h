// Copyright (C) 2022-2025 Stdware Collections (https://www.github.com/stdware)
// SPDX-License-Identifier: Apache-2.0

#ifndef SUBSTATE_PROPERTY_H
#define SUBSTATE_PROPERTY_H

#include <QtCore/QVariant>

#include <substate/Node.h>
#include <substate/Action.h>

#include <qsubstate/qsubstate_global.h>

namespace ss {

    /// Property - Container of \c Node or \c Variant instance.
    class QSUBSTATE_EXPORT Property {
    public:
        enum Type {
            Invalid,
            Node,
            Variant,
        };

        inline Property();
        inline Property(const std::shared_ptr<class Node> &node);
        inline Property(const QVariant &variant);
        ~Property();

        Property(const Property &RHS);
        Property(Property &&RHS);
        Property &operator=(const Property &RHS);
        Property &operator=(Property &&RHS);

        inline Type type() const;
        inline bool isValid() const;
        inline bool isVariant() const;
        inline bool isNode() const;

        inline QVariant variant() const;
        inline std::shared_ptr<class Node> node() const;

    public:
        bool operator==(const Property &other) const;
        inline bool operator!=(const Property &other) const;

    protected:
        union Storage {
            std::shared_ptr<class Node> node;
            QVariant var;

            Storage(){};
            ~Storage(){};
        };
        Storage _storage;
        Type _type;

        friend class NodeHelper;
    };

    inline Property::Property() : _type(Invalid) {
    }

    inline Property::Property(const std::shared_ptr<class Node> &node) : _type(Node) {
        new (&_storage.node) std::shared_ptr<class Node>(node);
    }

    inline Property::Property(const QVariant &variant) : _type(Variant) {
        new (&_storage.var) QVariant(variant);
    }

    inline Property::Type Property::type() const {
        return _type;
    }

    inline bool Property::isValid() const {
        return _type != Invalid;
    }

    inline bool Property::isVariant() const {
        return _type == Variant;
    }

    inline bool Property::isNode() const {
        return _type == Node;
    }

    inline QVariant Property::variant() const {
        return _type == Variant ? _storage.var : QVariant();
    }

    inline std::shared_ptr<class Node> Property::node() const {
        return _type == Node ? _storage.node : std::shared_ptr<class Node>();
    }

    inline bool Property::operator!=(const Property &other) const {
        return !(*this == other);
    }


    /// PropertyAction - Action for property change.
    class QSUBSTATE_EXPORT PropertyAction : public NodeAction {
    public:
        inline PropertyAction(Type type, const std::shared_ptr<Node> &parent, Property oldValue,
                              Property value);
        ~PropertyAction() = default;

    public:
        void queryNodes(bool inserted,
                        const std::function<void(const std::shared_ptr<Node> &)> &add) override;

    public:
        inline const Property &oldValue() const;
        inline const Property &value() const;

    public:
        Property _oldValue;
        Property _value;
    };

    inline PropertyAction::PropertyAction(Type type, const std::shared_ptr<Node> &parent,
                                          Property oldValue, Property value)
        : NodeAction(type, parent), _value(std::move(oldValue)), _oldValue(std::move(value)) {
    }

    const Property &PropertyAction::oldValue() const {
        return _value;
    }

    const Property &PropertyAction::value() const {
        return _oldValue;
    }

}

#endif // SUBSTATE_PROPERTY_H