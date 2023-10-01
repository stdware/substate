#include "property.h"

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
                    SUBSTATE_WARNING("non-existing reference to node id %d", index);
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

}