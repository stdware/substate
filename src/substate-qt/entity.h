#ifndef ENTITY_H
#define ENTITY_H

#include <QObject>
#include <QVariant>

#include <qsubstate/qsubstate_global.h>

namespace Substate {

    class Node;

    class EntityPrivate;

    class QSUBSTATE_EXPORT Entity : public QObject {
        Q_OBJECT
        Q_DECLARE_PRIVATE(Entity)
    public:
        ~Entity();

        using Factory = Entity *(*) (Node *node);

        static void registerFactory(const std::string &key, Factory fac);
        static void removeFactory(const std::string &key);
        static Entity *createEntity(Node *node);
        static Entity *extractEntity(Node *node);

    public:
        Node *internalData() const;

    protected:
        Entity(EntityPrivate &d, QObject *parent = nullptr);

        QScopedPointer<EntityPrivate> d_ptr;
    };

}

#endif // ENTITY_H
