#ifndef MEMENGINE_H
#define MEMENGINE_H

#include <substate/engine.h>

namespace Substate {

    class MemEnginePrivate;

    class SUBSTATE_EXPORT MemoryEngine : public Engine {
    public:
        MemoryEngine();
        ~MemoryEngine();

    public:
        void commit(const std::vector<Action *> &actions, const Variant &message) override;
        void execute(bool undo) override;

    protected:
        MemoryEngine(MemEnginePrivate &d);
    };

}

#endif // MEMENGINE_H
