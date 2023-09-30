#include "model.h"
#include "model_p.h"

#include <cassert>

#include "node_p.h"
#include "engines/memengine.h"

namespace Substate {

    ModelPrivate::ModelPrivate(Engine *engine) : engine(engine) {
        lockedNode = nullptr;
        maxIndex = 0;
    }

    ModelPrivate::~ModelPrivate() {
        delete engine;
    }

    void ModelPrivate::init() {
        Q_Q(Model);
        engine->setup(q);
    }

    int ModelPrivate::addIndex(Node *node, int idx) {
        int index = idx > 0 ? (maxIndex = std::max(maxIndex, idx), idx) : (++maxIndex);
        indexes.insert(std::make_pair(index, node));
        return index;
    }

    void ModelPrivate::removeIndex(int index) {
        indexes.erase(index);
    }

    void ModelPrivate::setRootItem_helper(Substate::Node *node) {
        Q_Q(Model);

        RootChangeAction a(node, root);

        // Pre-Propagate
        {
            ActionNotification n(Notification::ActionAboutToTrigger, &a);
            q->dispatch(&n);
        }

        // Do change
        if (root) {
            root->d_func()->setManaged(true);
        }
        if (node && node->isManaged()) {
            node->d_func()->setManaged(false);
        }
        root = node;

        // Propagate signal
        {
            ActionNotification n(Notification::ActionTriggered, &a);
            q->dispatch(&n);
        }
    }

    /*!
        \class Model
        \brief The model maintains the root node and interacts with the engine.
    */

    /*!
        Constructor. By default, the model will create an MemEngine as the backend
    */
    Model::Model() : Model(*new ModelPrivate(new MemoryEngine())) {
    }

    /*!
        Constructs with the given engine as the backend.
    */
    Model::Model(Engine *engine) : Model(*new ModelPrivate(engine)) {
    }

    /*!
        Destructor.
    */
    Model::~Model() {
    }

    /*!
        Returns the engine.
    */
    Engine *Model::engine() const {
        Q_D(const Model);
        return d->engine;
    }

    /*!
        Returns the model state.
    */
    Model::State Model::state() const {
        Q_D(const Model);
        return d->state;
    }

    /*!
        Returns \a true if the model is writable.

        The model is writable only if it's in transaction state and no node holds the meta-action
        lock.
    */
    bool Model::isWritable() const {
        Q_D(const Model);
        return d->state == Transaction && !d->lockedNode;
    }

    /*!
        Returns the node to the index points to if found.
    */
    Node *Model::indexOf(int index) const {
        Q_D(const Model);
        auto it = d->indexes.find(index);
        if (it == d->indexes.end())
            return nullptr;
        return it->second;
    }

    /*!
        Returns the root node of the model.
    */
    Node *Model::root() const {
        Q_D(const Model);
        return d->root;
    }

    /*!
        Sets the root node of the model.
    */
    void Model::setRoot(Node *node) {
        Q_D(Model);
        assert(isWritable());
        assert(!node || node->isFree());
        d->setRootItem_helper(node);
    }

    void Model::reset() {
        Q_D(Model);
        if (d->state != Idle) {
            SUBSTATE_FATAL("Attempt to reset at an invalid state");
        }

        Notification n(Notification::AboutToReset);
        dispatch(&n);

        d->engine->reset();
    }

    /*!
        Enters the transaction state.

        A fatal error will be raised if called when the engine is not in idle state.
    */
    void Model::beginTransaction() {
        Q_D(Model);
        if (d->state != Idle) {
            SUBSTATE_FATAL("Attempt to begin a transaction at an invalid state");
        }
        d->state = Transaction;
    }

    /*!
        Aborts the current transaction.

        A fatal error will be raised if called when the engine is not in transaction state.
    */
    void Model::abortTransaction() {
        Q_D(Model);
        if (d->state != Transaction) {
            SUBSTATE_FATAL("Cannot abort the transaction without an ongoing transaction");
        }

        auto &stack = d->txActions;
        for (auto it = stack.rbegin(); it != stack.rend(); ++it) {
            const auto &a = *it;
            a->execute(true);
            a->virtual_hook(Action::CleanNodesHook, nullptr); // Need to remove newly created items
            delete a;
        }
        stack.clear();

        d->state = Idle;
    }

    /*!
        Commits the current transaction.

        A fatal error will be raised if called when the engine is not in transaction state.
    */
    void Model::commitTransaction(const Engine::StepMessage &message) {
        Q_D(Model);
        if (d->state != Transaction) {
            SUBSTATE_FATAL("Cannot commit the transaction without an ongoing transaction");
        }
        if (d->txActions.empty())
            return;

        d->engine->commit(d->txActions, message);
        d->txActions.clear();

        d->state = Idle;

        Notification n(Notification::StepChange);
        dispatch(&n);
    }

    /*!
        Rollback to the previous step.
    */
    void Model::undo() {
        Q_D(Model);
        if (d->state != Idle) {
            SUBSTATE_FATAL("Attempt to undo at an invalid state");
        }
        d->state = Undo;
        d->engine->execute(true);
        d->state = Idle;

        Notification n(Notification::StepChange);
        dispatch(&n);
    }

    /*!
        Redo to the previous step.
    */
    void Model::redo() {
        Q_D(Model);
        if (d->state != Idle) {
            SUBSTATE_FATAL("Attempt to redo at an invalid state");
        }
        d->state = Redo;
        d->engine->execute(false);
        d->state = Idle;

        Notification n(Notification::StepChange);
        dispatch(&n);
    }

    int Model::minimumStep() const {
        Q_D(const Model);
        return d->engine->minimum();
    }

    int Model::maximumStep() const {
        Q_D(const Model);
        return d->engine->maximum();
    }

    int Model::currentStep() const {
        Q_D(const Model);
        return d->engine->current();
    }

    void Model::dispatch(Notification *n) {
        Sender::dispatch(n);

        Q_D(Model);
        switch (n->type()) {
            case Notification::ActionTriggered: {
                auto n2 = static_cast<ActionNotification *>(n);
                if (d->state == Transaction) {
                    d->txActions.push_back(n2->action()->clone());
                }
                break;
            }
            default:
                break;
        }
    }

    /*!
        \internal
    */
    Model::Model(ModelPrivate &d) : Sender(d) {
        d.init();
    }

}