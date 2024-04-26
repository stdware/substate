#ifndef ENTITY_P_H
#define ENTITY_P_H

#include <substate/node.h>

#include <qsubstate/entity.h>

namespace Substate {

    class QSUBSTATE_EXPORT EntityPrivate : public NodeExtra {
        Q_DECLARE_PUBLIC(Entity)
    public:
        EntityPrivate(Node *node);
        virtual ~EntityPrivate();
        void init();

        Entity *q_ptr;

        static EntityPrivate *get(Entity *q) {
            return q->d_func();
        }

        static const EntityPrivate *get(const Entity *q) {
            return q->d_func();
        }
    };

}

#endif // ENTITY_P_H
