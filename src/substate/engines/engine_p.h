#ifndef ENGINE_P
#define ENGINE_P

#include <vector>

#include <substate/engine.h>

namespace Substate {

    class SUBSTATE_EXPORT EnginePrivate {
        SUBSTATE_DECL_PUBLIC(Engine)
    public:
        EnginePrivate();
        virtual ~EnginePrivate();
        void init();
        Engine *q_ptr;

        Engine::State state;
        std::vector<Operation *> txOperations;
        int min, max, current;
    };

}

#endif // ENGINE_P
