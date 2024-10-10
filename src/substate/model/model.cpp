#include "model.h"
#include "model_p.h"

#include <cassert>
#include <utility>

#include "node_p.h"
#include "engines/memengine.h"
#include "nodehelper.h"

namespace Substate {

    ModelPrivate::ModelPrivate(Engine *engine) : engine(engine) {
    }

    ModelPrivate::~ModelPrivate() {
        // Remove all actions first (may hold the root reference)
        for (const auto &action : std::as_const(txActions)) {
            delete action;
        }

        // Remove engine (may hold the root reference)
        delete engine;

        NodeHelper::forceDelete(root);
    }

    void ModelPrivate::init() {
        QM_Q(Model);
        engine->setup(q);
    }

    void ModelPrivate::setRootItem_helper(Substate::Node *node) {
        QM_Q(Model);

        isChangingRoot = true;

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

        isChangingRoot = false;
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
        QM_D(const Model);
        return d->engine;
    }

    /*!
        Returns the model state.
    */
    Model::State Model::state() const {
        QM_D(const Model);
        return d->state;
    }

    /*!
        Returns \a true if the model is writable.

        The model is writable only if it's in transaction state and no node holds the meta-action
        lock.
    */
    bool Model::isWritable() const {
        QM_D(const Model);
        return d->state == Transaction && !d->lockedNode && !d->isChangingRoot;
    }

    /*!
        Returns the node to the index points to if found.
    */
    Node *Model::indexOf(int index) const {
        QM_D(const Model);
        return d->engine->indexOf(index);
    }

    /*!
        Returns the root node of the model.
    */
    Node *Model::root() const {
        QM_D(const Model);
        return d->root;
    }

    /*!
        Sets the root node of the model.
    */
    void Model::setRoot(Node *node) {
        QM_D(Model);
        assert(isWritable());
        assert(!node || node->isFree());
        d->setRootItem_helper(node);
    }

    void Model::reset() {
        QM_D(Model);
        if (d->state != Idle) {
            QMSETUP_FATAL("Attempt to reset at an invalid state");
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
        QM_D(Model);
        if (d->state != Idle) {
            QMSETUP_FATAL("Attempt to begin a transaction at an invalid state");
        }
        d->state = Transaction;

        d->engine->prepare();
    }

    /*!
        Aborts the current transaction.

        A fatal error will be raised if called when the engine is not in transaction state.
    */
    void Model::abortTransaction() {
        QM_D(Model);
        if (d->state != Transaction) {
            QMSETUP_FATAL("Cannot abort the transaction without an ongoing transaction");
        }

        auto &stack = d->txActions;
        for (auto it = stack.rbegin(); it != stack.rend(); ++it) {
            const auto &a = *it;
            a->execute(true);
            a->setState(Action::Detached); // Need to remove newly created items
            delete a;
        }
        stack.clear();

        d->engine->abort();

        d->state = Idle;
    }

    /*!
        Commits the current transaction with a message.

        A fatal error will be raised if called when the engine is not in transaction state.
    */
    void Model::commitTransaction(const Engine::StepMessage &message) {
        QM_D(Model);
        if (d->state != Transaction) {
            QMSETUP_FATAL("Cannot commit the transaction without an ongoing transaction");
        }
        if (d->txActions.empty()) {
            d->state = Idle;
            return;
        }

        d->engine->commit(d->txActions, message);
        d->txActions.clear();

        d->state = Idle;

        Notification n(Notification::StepChange);
        dispatch(&n);
    }

    /*!
       Returns the message of the given \a step.
    */
    Engine::StepMessage Model::stepMessage(int step) const {
        QM_D(const Model);
        return d->engine->stepMessage(step);
    }

    /*!
        Rollback to the previous step.
    */
    void Model::undo() {
        QM_D(Model);
        if (d->state != Idle) {
            QMSETUP_FATAL("Attempt to undo at an invalid state");
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
        QM_D(Model);
        if (d->state != Idle) {
            QMSETUP_FATAL("Attempt to redo at an invalid state");
        }
        d->state = Redo;
        d->engine->execute(false);
        d->state = Idle;

        Notification n(Notification::StepChange);
        dispatch(&n);
    }

    int Model::minimumStep() const {
        QM_D(const Model);
        return d->engine->minimum();
    }

    int Model::maximumStep() const {
        QM_D(const Model);
        return d->engine->maximum();
    }

    int Model::currentStep() const {
        QM_D(const Model);
        return d->engine->current();
    }

    void Model::dispatch(Notification *n) {
        Sender::dispatch(n);

        QM_D(Model);
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