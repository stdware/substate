#include "property.h"

#include "substateglobal_p.h"
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

    Property Property::read(IStream &stream) {
        int32_t type;
        stream >> type;

        Property value;
        switch (type) {
            case PropertyFlag::InvalidValue:
                break;
            case PropertyFlag::NodeValue: {
                int index;
                stream >> index;
                value = reinterpret_cast<Node *>(uintptr_t(index));
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
        if (s == Detached) {
            if (v.isNode()) {
                delete v.node();
            }
        } else if (s == Deleted) {
            if (v.isNode() && v.node()->isManaged()) {
                NodeHelper::forceDelete(v.node());
            }
        }
    }

    void PropertyAction::virtual_hook(int id, void *data) {
        switch (id) {
            case DetachHook: {
                if (v.isNode()) {
                    v = NodeHelper::clone(v.node(), false);
                }
                return;
            }
            case InsertedNodesHook: {
                if (v.isNode()) {
                    auto &res = *reinterpret_cast<std::vector<Node *> *>(data);
                    res.push_back(v.node());
                }
                return;
            }
            case RemovedNodesHook: {
                if (oldv.isNode()) {
                    auto &res = *reinterpret_cast<std::vector<Node *> *>(data);
                    res.push_back(oldv.node());
                }
                return;
            }
            case DeferredReferenceHook: {
                SUBSTATE_FIND_DEFERRED_REFERENCE_NODE(data, m_parent, m_parent)

                // Find new node
                if (v.isNode()) {
                    SUBSTATE_FIND_DEFERRED_REFERENCE_NODE(data, v.node(), v)
                }

                // Find old node
                if (oldv.isNode()) {
                    SUBSTATE_FIND_DEFERRED_REFERENCE_NODE(data, oldv.node(), oldv)
                }
                return;
            }
            default:
                break;
        }
    }

}