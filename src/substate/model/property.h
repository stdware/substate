#ifndef PROPERTY_H
#define PROPERTY_H

#include <variant>

#include <substate/node.h>
#include <substate/variant.h>

namespace Substate {

    class SUBSTATE_EXPORT Property {
    public:
        inline Property();
        inline Property(Node *node);
        inline Property(const Variant &variant);

        inline bool isValid() const;
        inline bool isVariant() const;
        inline bool isNode() const;

        inline const Variant &variant() const;
        inline Node *node() const;

    public:
        inline bool operator==(const Property &other) const;
        inline bool operator!=(const Property &other) const;

    public:
        static Property read(IStream &stream, const std::unordered_map<int, Node *> &existingNodes);
        void write(OStream &stream) const;

    private:
        std::variant<std::monostate, Node *, Variant> var;
    };

    inline Property::Property() = default;

    inline Property::Property(Node *node) {
        if (node)
            var = node;
    }

    inline Property::Property(const Variant &variant) {
        var = variant;
    }

    inline bool Property::isValid() const {
        return var.index() > 0;
    }

    inline bool Property::isVariant() const {
        return var.index() == 2;
    }

    inline bool Property::isNode() const {
        return var.index() == 1;
    }

    inline const Variant &Property::variant() const {
        return var.index() == 2 ? std::get<Variant>(var) : Variant::sharedNull();
    }

    inline Node *Property::node() const {
        return var.index() == 1 ? std::get<Node *>(var) : nullptr;
    }

    bool Property::operator==(const Property &other) const {
        return var == other.var;
    }

    bool Property::operator!=(const Property &other) const {
        return var != other.var;
    }

}

#endif // PROPERTY_H
