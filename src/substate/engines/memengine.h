#ifndef MEMENGINE_H
#define MEMENGINE_H

#include <substate/engine.h>

namespace Substate {

    class MemEnginePrivate;

    class SUBSTATE_EXPORT MemEngine : public Engine {
    public:
        MemEngine();
        ~MemEngine();

    public:
        void commit(const std::vector<Action *> &actions, const Variant &message) override;
        void execute(bool undo) override;

    protected:
        MemEngine(MemEnginePrivate &d);
    };

}

#endif // MEMENGINE_H
