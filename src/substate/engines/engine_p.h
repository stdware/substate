#ifndef ENGINE_P_H
#define ENGINE_P_H

#include <vector>
#include <unordered_map>

#include <substate/engine.h>

namespace Substate {

    class SUBSTATE_EXPORT EnginePrivate {
        QMSETUP_DECL_PUBLIC(Engine)
    public:
        EnginePrivate();
        virtual ~EnginePrivate();
        void init();
        Engine *q_ptr;

        Model *model;
    };

}

#endif // ENGINE_P_H
