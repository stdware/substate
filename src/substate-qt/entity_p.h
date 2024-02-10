#ifndef ENTITY_P_H
#define ENTITY_P_H

#include <substate/node.h>

#include <qsubstate/entity.h>

namespace Substate {

    extern const std::string entity_dyn_key;

    class QSUBSTATE_EXPORT EntityPrivate : public NodeExtra {
        Q_DECLARE_PUBLIC(Entity)
    public:
        EntityPrivate(Node *node);
        virtual ~EntityPrivate();
        void init();

        Entity *q_ptr;
    };

}

#endif // ENTITY_P_H
