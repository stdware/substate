#include "engine.h"
#include "engine_p.h"

namespace Substate {

    EnginePrivate::EnginePrivate() {
        min = 0;
        max = 0;
        current = 0;
    }

    EnginePrivate::~EnginePrivate() {
    }

    void EnginePrivate::init() {
    }

    /*!
        \class Engine

        Engine manages the internal state of the model and the lifecycle of the nodes.
    */

    /*!
        Destructor.
    */
    Engine::~Engine() {
    }

    Engine::State Engine::state() const {
        Q_D(const Engine);
        return d->state;
    }

    void Engine::beginTransaction() {
        Q_D(Engine);
        if (d->state != Idle) {
            SUBSTATE_ERROR("Attempt to begin a transaction at an invalid state.\n");
            return;
        }
        d->state = Transaction;
    }

    void Engine::abortTransaction() {
        Q_D(Engine);
        if (d->state != Transaction) {
            SUBSTATE_ERROR("Cannot abort the transaction without an ongoing transaction.\n");
            return;
        }
    }

    void Engine::commitTransaction(const Variant &message) {
        Q_D(Engine);
        if (d->state != Transaction) {
            SUBSTATE_ERROR("Cannot commit the transaction without an ongoing transaction.\n");
            return;
        }
        commited(d->txOperations, message);
        d->txOperations.clear();
    }

    void Engine::commitOperation(Operation *op) {
        Q_D(Engine);
        if (d->state != Transaction) {
            SUBSTATE_ERROR("Cannot commit the operation without an ongoing transaction.\n");
            return;
        }
        d->txOperations.push_back(op);
    }

    int Engine::minimum() const {
        Q_D(const Engine);
        return d->min;
    }

    int Engine::maximum() const {
        Q_D(const Engine);
        return d->max;
    }

    int Engine::current() const {
        Q_D(const Engine);
        return d->current;
    }

    Engine::Engine(EnginePrivate &d) : d_ptr(&d) {
        d.q_ptr = this;
        d.init();
    }

}