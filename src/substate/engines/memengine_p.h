#ifndef MEMENGINE_P_H
#define MEMENGINE_P_H

#include <substate/private/engine_p.h>
#include <substate/memengine.h>

namespace Substate {

    class SUBSTATE_EXPORT MemoryEnginePrivate : public EnginePrivate {
        SUBSTATE_DECL_PUBLIC(MemoryEngine)
    public:
        MemoryEnginePrivate();
        ~MemoryEnginePrivate();
        void init();

        int maxSteps;
        int min;
        int current;

        struct TransactionData {
            std::vector<Action *> actions;
            Engine::StepMessage message;
        };
        std::vector<TransactionData> stack;

        void removeActions(size_t begin, size_t end);

        virtual bool acceptChangeMaxSteps(int steps) const;
        virtual void afterCurrentChange();
        virtual void afterCommit(const std::vector<Action *> &actions,
                                 const Engine::StepMessage &message);
        virtual void afterReset();
    };

}

#endif // MEMENGINE_P_H
