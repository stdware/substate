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

    /*!
        Returns the minimum step the engine can reach by executing undo.
    */
    int Engine::minimum() const {
        Q_D(const Engine);
        return d->min;
    }

    /*!
        Returns the maximum step the engine can reach by executing redo.
    */
    int Engine::maximum() const {
        Q_D(const Engine);
        return d->max;
    }

    /*!
        Returns the current step.
    */
    int Engine::current() const {
        Q_D(const Engine);
        return d->current;
    }

    /*!
        Sets the minimum step.
    */
    void Engine::setMinimum(int value) {
        Q_D(Engine);
        d->min = value;
    }

    /*!
        Sets the maximum step.
    */
    void Engine::setMaximum(int value) {
        Q_D(Engine);
        d->max = value;
    }

    /*!
        Sets the current step.
    */
    void Engine::setCurrent(int value) {
        Q_D(Engine);
        d->current = value;
    }

    /*!
        \internal
    */
    Engine::Engine(EnginePrivate &d) : d_ptr(&d) {
        d.q_ptr = this;
        d.init();
    }

}