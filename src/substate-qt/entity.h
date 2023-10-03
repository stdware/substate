#ifndef ENTITY_H
#define ENTITY_H

#include <QObject>

#include <substate/node.h>
#include <qsubstate/qsubstate_global.h>

namespace Substate {

    class EntityPrivate;

    class QSUBSTATE_EXPORT Entity : public QObject, public NodeExtra {
        Q_OBJECT
        Q_DECLARE_PRIVATE(Entity)
    public:
        ~Entity();

    protected:
        void notified(Notification *n) override;

    protected:
        Entity(EntityPrivate &d, QObject *parent = nullptr);

        QScopedPointer<EntityPrivate> d_ptr;
    };

}

#endif // ENTITY_H
