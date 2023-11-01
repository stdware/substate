#include "property.h"

#include "nodehelper.h"

namespace Substate {

    enum PropertyFlag : int32_t {
        InvalidValue,
        NodeValue,
        VariantValue,
    };

    /*!
        \class Property
        \brief Property stores a node or a variant pointer.
    */

    Property Property::read(IStream &stream, const std::unordered_map<int, Node *> &existingNodes) {
        int32_t type;
        stream >> type;

        Property value;
        switch (type) {
            case PropertyFlag::InvalidValue:
                break;
            case PropertyFlag::NodeValue: {
                int index;
                stream >> index;

                auto it = existingNodes.find(index);
                if (it == existingNodes.end()) {
                    QTMEDIATE_WARNING("non-existing reference to node id %d", index);
                    stream.setState(std::ios::failbit);
                    break;
                }
                value = it->second;
                break;
            }
            case PropertyFlag::VariantValue: {
                Variant var;
                stream >> var;
                if (stream.fail()) {
                    break;
                }
                value = var;
                break;
            }
            default:
                stream.setState(std::ios::badbit);
                break;
        }
        return value;
    }

    void Property::write(OStream &stream) const {
        const auto &value = *this;
        if (!value.isValid()) {
            stream << PropertyFlag::InvalidValue;
        } else if (value.isNode()) {
            stream << PropertyFlag::NodeValue;
            stream << value.node()->index();
        } else {
            stream << PropertyFlag::VariantValue;
            stream << value.variant();
        }
    }

    IStream &operator>>(IStream &stream, Property &value) {
        value = {};

        int32_t type;
        stream >> type;
        switch (type) {
            case PropertyFlag::InvalidValue:
                break;
            case PropertyFlag::NodeValue: {
                value = Node::read(stream);
                break;
            }
            case PropertyFlag::VariantValue: {
                Variant var;
                stream >> var;
                if (stream.fail()) {
                    break;
                }
                value = var;
                break;
            }
            default:
                stream.setState(std::ios::badbit);
                break;
        }
        return stream;
    }

    OStream &operator<<(OStream &stream, const Property &value) {
        if (!value.isValid()) {
            stream << PropertyFlag::InvalidValue;
        } else if (value.isNode()) {
            stream << PropertyFlag::NodeValue;
            value.node()->write(stream);
        } else {
            stream << PropertyFlag::VariantValue;
            stream << value.variant();
        }
        return stream;
    }

    PropertyAction::PropertyAction(Type type, Node *parent, const Property &value,
                                   const Property &oldValue)
        : NodeAction(type, parent), v(value), oldv(oldValue) {
    }

    PropertyAction::~PropertyAction() {
    }

    void PropertyAction::virtual_hook(int id, void *data) {
        switch (id) {
            case CleanNodesHook: {
                if (v.isNode()) {
                    NodeHelper::forceDelete(v.node());
                }
                break;
            }
            case InsertedNodesHook: {
                if (v.isNode()) {
                    auto &res = *reinterpret_cast<std::vector<Node *> *>(data);
                    res.push_back(v.node());
                }
                break;
            }
            case RemovedNodesHook: {
                if (oldv.isNode()) {
                    auto &res = *reinterpret_cast<std::vector<Node *> *>(data);
                    res.push_back(oldv.node());
                }
                break;
            }
            default:
                break;
        }
    }

}