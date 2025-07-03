#include "Property.h"

namespace ss {

    Property::Property(const Property &RHS) : _type(RHS._type) {
        switch (_type) {
            case Node:
                new (&_storage.node) std::shared_ptr<class Node>(RHS._storage.node);
                break;
            case Variant:
                new (&_storage.var) QVariant(RHS._storage.var);
                break;
            default:
                break;
        }
    }

    Property::Property(Property &&RHS) : _type(RHS._type) {
        switch (_type) {
            case Node:
                new (&_storage.node) std::shared_ptr<class Node>(std::move(RHS._storage.node));
                break;
            case Variant:
                new (&_storage.var) QVariant(std::move(RHS._storage.var));
                break;
            default:
                break;
        }
    }

    Property &Property::operator=(const Property &RHS) {
        if (this != &RHS) {
            switch (_type) {
                case Node:
                    _storage.node.~shared_ptr();
                    break;
                case Variant:
                    _storage.var.~QVariant();
                    break;
                default:
                    break;
            }
            _type = RHS._type;
            switch (_type) {
                case Node:
                    new (&_storage.node) std::shared_ptr<class Node>(RHS._storage.node);
                    break;
                case Variant:
                    new (&_storage.var) QVariant(RHS._storage.var);
                    break;
                default:
                    break;
            }
        }
        return *this;
    }

    Property &Property::operator=(Property &&RHS) {
        if (this != &RHS) {
            switch (_type) {
                case Node:
                    _storage.node.~shared_ptr();
                    break;
                case Variant:
                    _storage.var.~QVariant();
                    break;
                default:
                    break;
            }
            _type = RHS._type;
            switch (_type) {
                case Node:
                    new (&_storage.node) std::shared_ptr<class Node>(std::move(RHS._storage.node));
                    break;
                case Variant:
                    new (&_storage.var) QVariant(std::move(RHS._storage.var));
                    break;
                default:
                    break;
            }
        }
        return *this;
    }

    Property::~Property() {
        switch (_type) {
            case Node:
                _storage.node.~shared_ptr();
                break;
            case Variant:
                _storage.var.~QVariant();
                break;
            default:
                break;
        }
    }

    bool Property::operator==(const Property &other) const {
        if (_type != other._type)
            return false;
        switch (_type) {
            case Node:
                return _storage.node == other._storage.node;
            case Variant:
                return _storage.var == other._storage.var;
            default:
                break;
        }
        return true;
    }

    void PropertyAction::queryNodes(bool inserted,
                                    const std::function<void(const std::shared_ptr<Node> &)> &add) {
        if (inserted) {
            if (_oldValue.isNode()) {
                add(_oldValue.node());
            }
        } else {
            if (_value.isNode()) {
                add(_value.node());
            }
        }
    }

}