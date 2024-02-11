#include "structentity.h"

#include <substate/structnode.h>
#include <substate/private/structnode_p.h>
#include <substate/nodehelper.h>

#include "entity_p.h"
#include "qsubstateglobal_p.h"

namespace Substate {

    static const QVariant *prop2variant(const Property &prop) {
        return reinterpret_cast<const QVariant *>(prop.variant().constData());
    }

    class StructEntityBasePrivate : public EntityPrivate {
        Q_DECLARE_PUBLIC(StructEntityBase)
    public:
        StructEntityBasePrivate(Node *node) : EntityPrivate(node) {
        }

        ~StructEntityBasePrivate() = default;

        void notified(Notification *n) override {
            switch (n->type()) {
                case Notification::ActionTriggered: {
                    auto n2 = static_cast<ActionNotification *>(n);
                    auto a = n2->action();
                    switch (a->type()) {
                        case Action::StructAssign: {
                            auto a2 = static_cast<StructAction *>(a);
                            sendAssigned(a2->index(), a2->value(), a2->oldValue());
                            break;
                        }
                        default:
                            break;
                    }
                    break;
                }

                default:
                    break;
            }
        }

        inline void sendAssigned(int index, const Property &prop, const Property &oldProp) {
            Q_Q(StructEntityBase);

            StructEntityBase::Value val;
            StructEntityBase::Value oldVal;

            if (prop.isNode()) {
                val.is_variant = false;
                val.item = external ? Entity::extractEntity(prop.node())
                                    : Entity::createEntity(prop.node());
            } else {
                val.is_variant = true;
                val.variant = prop2variant(prop);
            }

            if (oldProp.isNode()) {
                oldVal.is_variant = false;
                oldVal.item = Entity::extractEntity(prop.node());
            } else {
                oldVal.is_variant = true;
                oldVal.variant = prop2variant(oldProp);
            }

            q->sendAssigned(index, val, oldVal);

            if (oldProp.isNode()) {
                auto node = oldProp.node();
                if (!node->isFree()) {
                    delete Entity::extractEntity(oldProp.node());
                }
            }
        }

        bool external = false;
    };

    StructEntityBase::~StructEntityBase() {
    }

    StructEntityBase::Value StructEntityBase::valueImpl(int i) const {
        Q_D(const StructEntityBase);

        const auto &prop =
            static_cast<StructNodePrivate *>(NodeHelper::get(d->internalData()))->array.at(i);
        Value res;
        if (prop.isNode()) {
            res.is_variant = false;
            res.item = Entity::extractEntity(prop.node());
        } else {
            res.is_variant = true;
            res.variant = prop2variant(prop);
        }
        return res;
    }

    bool StructEntityBase::setValueImpl(int i, const Value &value) {
        Q_D(StructEntityBase);
        d->external = true;
        auto res = static_cast<StructNode *>(d->internalData())
                       ->assign(i, value.is_variant ? Property(Variant::fromValue(*value.variant))
                                                    : Property(value.item->internalData()));
        d->external = false;
        return res;
    }

    int StructEntityBase::sizeImpl() const {
        Q_D(const StructEntityBase);
        return static_cast<StructNode *>(d->internalData())->size();
    }

    StructEntityBase::StructEntityBase(Node *node, QObject *parent)
        : Entity(*new StructEntityBasePrivate(node), parent) {
    }

}