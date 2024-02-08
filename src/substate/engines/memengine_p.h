#ifndef MEMENGINE_P_H
#define MEMENGINE_P_H

#include <substate/private/engine_p.h>
#include <substate/memengine.h>

namespace Substate {

    class SUBSTATE_EXPORT MemoryEnginePrivate : public EnginePrivate {
        QMSETUP_DECL_PUBLIC(MemoryEngine)
    public:
        MemoryEnginePrivate();
        ~MemoryEnginePrivate();
        void init();

        // The maximum number of actions kept in memory
        int maxSteps = 4; // For test only

        // The minimum step the engine can reach
        int min = 0;

        // The current index of undo stack
        int current = 0;

        // Undo stack
        struct TransactionData {
            std::vector<Action *> actions;
            Engine::StepMessage message;
        };
        std::vector<TransactionData> stack;

        // Remove the specified range of actions in undo stack from memory
        void removeActions(int begin, int end);

        // Determine if the given preserved step number is acceptable
        virtual bool acceptChangeMaxSteps(int steps) const;

        // Do something after undo or redo
        virtual void afterCurrentChange();

        // Do something after commit
        virtual void afterCommit(const std::vector<Action *> &actions,
                                 const Engine::StepMessage &message);

        // Do something after reset
        virtual void afterReset();
    };

}

#endif // MEMENGINE_P_H
