#ifndef ENTITY_H
#define ENTITY_H

#include <QObject>
#include <QVariant>

#include <substate/node.h>
#include <qsubstate/qsubstateglobal.h>

namespace Substate {

    class EntityPrivate;

    class QSUBSTATE_EXPORT Entity : public QObject {
        Q_OBJECT
        Q_DECLARE_PRIVATE(Entity)
    public:
        ~Entity();

        using Factory = Entity *(*) (Node *node, QObject *parent);

        static void registerFactory(const std::string &key, Factory fac);
        static void removeFactory(const std::string &key);
        static Entity *createEntity(Node *node);
        static Entity *extractEntity(Node *node);

    public:
        Node *internalData() const;

    protected:
        struct Value {
            bool is_variant;
            union {
                Entity *item;
                const QVariant *variant;
            };

            inline Value() : is_variant(false) {
                item = nullptr;
            }

            inline Value(const QVariant &var) : is_variant(true) {
                variant = &var;
            }

            inline Value(Entity *item) : is_variant(false) {
                this->item = item;
            }
        };

        QVariant dynamicDataImpl(const QByteArray &key) const;
        void setDynamicDataImpl(const QByteArray &key, const QVariant &value);

    protected:
        Entity(EntityPrivate &d, QObject *parent = nullptr);

        QScopedPointer<EntityPrivate> d_ptr;
    };

}

#endif // ENTITY_H
