#include "mappingentity.h"

#include <substate/mappingnode.h>
#include <substate/private/mappingnode_p.h>
#include <substate/nodehelper.h>

#include "entity_p.h"
#include "qsubstateglobal_p.h"

namespace Substate {

    static const QVariant *prop2variant(const Property &prop) {
        return reinterpret_cast<const QVariant *>(prop.variant().constData());
    }

    class MappingEntityBasePrivate : public EntityPrivate {
        Q_DECLARE_PUBLIC(MappingEntityBase)
    public:
        MappingEntityBasePrivate(Node *node) : EntityPrivate(node) {
        }

        ~MappingEntityBasePrivate() = default;

        void notified(Notification *n) override {
            switch (n->type()) {
                case Notification::ActionTriggered: {
                    auto n2 = static_cast<ActionNotification *>(n);
                    auto a = n2->action();
                    switch (a->type()) {
                        case Action::StructAssign: {
                            auto a2 = static_cast<MappingAction *>(a);
                            sendInserted(a2->key(), a2->value(), a2->oldValue());
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

        inline void sendInserted(const std::string &key, const Property &prop,
                                 const Property &oldProp) {
            Q_Q(MappingEntityBase);

            MappingEntityBase::Value val;
            MappingEntityBase::Value oldVal;

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

            q->sendInserted(QByteArray::fromStdString(key), val, oldVal);

            if (oldProp.isNode()) {
                auto node = oldProp.node();
                if (!node->isFree()) {
                    delete Entity::extractEntity(oldProp.node());
                }
            }
        }

        bool external = false;
    };

    MappingEntityBase::~MappingEntityBase() {
    }

    MappingEntityBase::Value MappingEntityBase::valueImpl(const QByteArray &key) const {
        Q_D(const MappingEntityBase);

        const auto &map =
            static_cast<MappingNodePrivate *>(NodeHelper::get(d->internalData()))->mapping;
        auto it = map.find(key.toStdString());
        if (it == map.end())
            return {false, nullptr};

        auto &prop = it->second;

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

    bool MappingEntityBase::setValueImpl(const QByteArray &key, const Value &value) {
        Q_D(MappingEntityBase);
        d->external = true;
        auto res = static_cast<MappingNode *>(d->internalData())
                       ->setProperty(key.toStdString(),
                                     value.is_variant ? Property(Variant::fromValue(*value.variant))
                                                      : Property(value.item->internalData()));
        d->external = false;
        return res;
    }

    QList<QByteArray> MappingEntityBase::keysImpl() const {
        Q_D(const MappingEntityBase);
        const auto &arr = static_cast<MappingNode *>(d->internalData())->keys();

        QList<QByteArray> res;
        res.reserve(arr.size());
        for (const auto &item : arr) {
            res.append(QByteArray::fromStdString(item));
        }
        return res;
    }

    int MappingEntityBase::sizeImpl() const {
        Q_D(const MappingEntityBase);
        return static_cast<MappingNode *>(d->internalData())->size();
    }

    MappingEntityBase::MappingEntityBase(Node *node, QObject *parent)
        : Entity(*new MappingEntityBasePrivate(node), parent) {
    }

}