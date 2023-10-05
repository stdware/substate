#ifndef ENTITY_H
#define ENTITY_H

#include <QObject>

#include <substate/node.h>
#include <qsubstate/qsubstate_global.h>

namespace Substate {

    class EntityPrivate;

    class QSUBSTATE_EXPORT Entity : public QObject {
        Q_OBJECT
        Q_DECLARE_PRIVATE(Entity)
    public:
        ~Entity();

    public:
        Node *internalData(Node *node) const;

    protected:
        Entity(EntityPrivate &d, QObject *parent = nullptr);

        QScopedPointer<EntityPrivate> d_ptr;
    };

}

#endif // ENTITY_H
