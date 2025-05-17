#ifndef ENGINE_P_H
#define ENGINE_P_H

#include <vector>
#include <unordered_map>

#include <substate/engine.h>

namespace Substate {

    class SUBSTATE_EXPORT EnginePrivate {
        SUBSTATE_DECL_PUBLIC(Engine)
    public:
        EnginePrivate();
        virtual ~EnginePrivate();
        void init();
        Engine *q_ptr;

        Model *model;
        std::unordered_map<int, Node *> indexes;
        int maxIndex;

        int addIndex(Node *node, int idx = 0);
        void removeIndex(int index);
    };

}

#endif // ENGINE_P_H
