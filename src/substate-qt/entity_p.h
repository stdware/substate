#ifndef ENTITY_P_H
#define ENTITY_P_H

#include <qsubstate/entity.h>

namespace Substate {

    class QSUBSTATE_EXPORT EntityPrivate {
        Q_DECLARE_PUBLIC(Entity)
    public:
        EntityPrivate();
        virtual ~EntityPrivate();
        void init();
        Entity *q_ptr;
    };

}

#endif // ENTITY_P_H