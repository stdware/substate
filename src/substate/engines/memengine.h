#ifndef MEMENGINE_H
#define MEMENGINE_H

#include <substate/engine.h>

namespace Substate {

    class MemoryEnginePrivate;

    class SUBSTATE_EXPORT MemoryEngine : public Engine {
        QMSETUP_DECL_PRIVATE(MemoryEngine)
    public:
        MemoryEngine();
        ~MemoryEngine();

    public:
        int preservedSteps() const;
        void setPreservedSteps(int steps);

    public:
        void commit(const std::vector<Action *> &actions, const StepMessage &message) override;
        void execute(bool undo) override;
        void reset() override;

        int minimum() const override;
        int maximum() const override;
        int current() const override;
        StepMessage stepMessage(int step) const override;

    protected:
        MemoryEngine(MemoryEnginePrivate &d);
    };

}

#endif // MEMENGINE_H
