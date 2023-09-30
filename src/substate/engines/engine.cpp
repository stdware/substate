#include "engine.h"
#include "engine_p.h"

#include "model/nodehelper.h"
#include "model/model_p.h"

namespace Substate {

    EnginePrivate::EnginePrivate() {
        model = nullptr;
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
        Sets up the engine with the specified model.
    */
    void Engine::setup(Model *model) {
        Q_D(Engine);
        d->model = model;
    }

    /*!
        Commits a list of actions with a message to the engine.
    */
    void Engine::commit(const std::vector<Action *> &actions, const StepMessage &message) {
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
        \fn void Engine::execute(bool undo)

        Executes undo or redo and updates engine's internal state.
    */

    /*!
        Resets the model.
    */
    void Engine::reset() {
        Q_D(Engine);

        auto model_d = d->model->d_func();

        // Skip removing index when deleting item to speed up
        model_d->is_clearing = true;

        // Remove all items
        std::list<Node *> rootItems;
        for (const auto &pair : std::as_const(model_d->indexes)) {
            if (!pair.second->parent()) {
                rootItems.push_back(pair.second);
            }
        }
        for (const auto &node : std::as_const(rootItems)) {
            NodeHelper::forceDelete(node);
        }

        // Remove root item
        model_d->root = nullptr;

        model_d->is_clearing = false;
        model_d->indexes.clear();
        model_d->maxIndex = 0;
    }

    /*!
        \fn int Engine::minimum() const

        Returns the minimum step the engine can reach by executing undo.
    */

    /*!
        \fn int Engine::maximum() const

        Returns the maximum step the engine can reach by executing redo.
    */

    /*!
        \fn int Engine::current() const

        Returns the current step.
    */

    /*!
        \internal
    */
    Engine::Engine(EnginePrivate &d) : d_ptr(&d) {
        d.q_ptr = this;
        d.init();
    }

}