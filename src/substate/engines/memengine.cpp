#include "memengine.h"
#include "memengine_p.h"

namespace Substate {

    MemEnginePrivate::MemEnginePrivate() {
    }

    MemEnginePrivate::~MemEnginePrivate() {
    }

    void MemEnginePrivate::init() {
    }

    MemEngine::MemEngine() : MemEngine(*new MemEnginePrivate()) {
    }

    MemEngine::~MemEngine() {
    }

    void MemEngine::commit(const std::vector<Action *> &actions, const Variant &message) {
        Engine::commit(actions, message);
    }

    void MemEngine::execute(bool undo) {
    }

    /*!
        \internal
    */
    MemEngine::MemEngine(MemEnginePrivate &d) : Engine(d) {
        d.init();
    }
}