#include "memengine.h"
#include "memengine_p.h"

namespace Substate {

    MemEnginePrivate::MemEnginePrivate() {
    }

    MemEnginePrivate::~MemEnginePrivate() {
    }

    void MemEnginePrivate::init() {
    }

    MemoryEngine::MemoryEngine() : MemoryEngine(*new MemEnginePrivate()) {
    }

    MemoryEngine::~MemoryEngine() {
    }

    void MemoryEngine::commit(const std::vector<Action *> &actions, const Variant &message) {
        Engine::commit(actions, message);
    }

    void MemoryEngine::execute(bool undo) {
    }

    /*!
        \internal
    */
    MemoryEngine::MemoryEngine(MemEnginePrivate &d) : Engine(d) {
        d.init();
    }
}