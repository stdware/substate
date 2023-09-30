#ifndef MEMENGINE_P_H
#define MEMENGINE_P_H

#include <substate/private/engine_p.h>
#include <substate/memengine.h>

namespace Substate {

    class SUBSTATE_EXPORT MemEnginePrivate : public EnginePrivate {
    public:
        MemEnginePrivate();
        ~MemEnginePrivate();
        void init();
    };

}

#endif // MEMENGINE_P_H
