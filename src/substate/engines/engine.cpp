#include "engine.h"
#include "engine_p.h"

#include "model/nodehelper.h"

namespace Substate {

    EnginePrivate::EnginePrivate() {
        model = nullptr;
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
        \brief Engine manages the internal state of the model and the lifecycle of the nodes.
    */

    /*!
        Destructor.
    */
    Engine::~Engine() {
    }

    /*!
        Returns the model related to the engine.
    */
    Model *Engine::model() const {
        Q_D(const Engine);
        return d->model;
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
        Sets up the engine with the specified model.
    */
    void Engine::setup(Model *model) {
        Q_D(Engine);
        d->model = model;
    }

    /*!
        Commits a list of actions with a message to the engine.
    */
    void Engine::commit(const std::vector<Action *> &actions, const Variant &message) {
        Q_D(Engine);

        // Collect inserted nodes and get ownership
        std::vector<Node *> nodes;
        for (auto a : actions) {
            std::vector<Node *> curNodes;
            a->virtual_hook(Action::InsertedNodesHook, &curNodes);
            nodes.insert(nodes.end(), curNodes.begin(), curNodes.end());
        }
        for (auto n : std::as_const(nodes)) {
            NodeHelper::propagateModel(n, d->model);
        }
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