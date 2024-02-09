#include "engine.h"
#include "engine_p.h"

#include "model/nodehelper.h"
#include "model/model_p.h"

namespace Substate {

    EnginePrivate::EnginePrivate() {
        model = nullptr;
        maxIndex = 0;
    }

    EnginePrivate::~EnginePrivate() {
    }

    void EnginePrivate::init() {
    }

    int EnginePrivate::addIndex(Node *node, int idx) {
        int index = idx > 0 ? (maxIndex = std::max(maxIndex, idx), idx) : (++maxIndex);
        indexes.insert(std::make_pair(index, node));
        return index;
    }

    void EnginePrivate::removeIndex(int index) {
        indexes.erase(index);
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
        QM_D(const Engine);
        return d->model;
    }

    /*!
        Returns the node to the index points to if found.
    */
    Node *Engine::indexOf(int index) const {
        QM_D(const Engine);
        auto it = d->indexes.find(index);
        if (it == d->indexes.end())
            return nullptr;
        return it->second;
    }

    /*!
        Sets up the engine with the specified model.
    */
    void Engine::setup(Model *model) {
        QM_D(Engine);
        d->model = model;
    }

    /*!
        Prepare for transaction, this function is called when the model turns into transaction mode.
    */
    void Engine::prepare() {
    }

    /*!
        Abort transaction, this function is called when the model turns into idle mode without any
        actions to commit.
    */
    void Engine::abort() {
    }

    /*!
        Commits a list of actions with a message to the engine.
    */
    void Engine::commit(const std::vector<Action *> &actions, const StepMessage &message) {
        QM_D(Engine);

        // Collect inserted nodes and get ownership
        std::vector<Node *> nodes;
        for (auto a : actions) {
            std::vector<Node *> curNodes;
            a->virtual_hook(Action::InsertedNodesHook, &curNodes);
            nodes.insert(nodes.end(), curNodes.begin(), curNodes.end());
        }
        for (auto n : std::as_const(nodes)) {
            NodeHelper::propagateEngine(n, this);
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
        QM_D(Engine);

        auto model_d = d->model->d_func();

        // Skip removing index when deleting item to speed up
        model_d->is_clearing = true;

        // Remove all items
        std::list<Node *> rootItems;
        for (const auto &pair : std::as_const(d->indexes)) {
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
        d->indexes.clear();
        d->maxIndex = 0;
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