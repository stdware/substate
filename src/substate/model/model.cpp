#include "model.h"
#include "model_p.h"

namespace Substate {

    ModelPrivate::ModelPrivate() {
        engine = nullptr;
        lockedNode = nullptr;
        maxIndex = 0;
    }

    ModelPrivate::~ModelPrivate() {
    }

    void ModelPrivate::init() {
    }

    int ModelPrivate::addIndex(Node *node, int idx) {
        int index = idx > 0 ? (maxIndex = std::max(maxIndex, idx), idx) : (++maxIndex);
        indexes.insert(std::make_pair(index, node));
        return index;
    }

    void ModelPrivate::removeIndex(int index) {
        indexes.erase(index);
    }

    Model::Model() : Model(*new ModelPrivate()) {
    }

    Model::~Model() {
    }

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
        Enters the transaction state.

        A fatal error will be raised if called when the engine is not in idle state.
    */
    void Model::beginTransaction() {
        Q_D(Model);
        if (d->state != Idle) {
            SUBSTATE_FATAL("Attempt to begin a transaction at an invalid state.\n");
            return;
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
            SUBSTATE_FATAL("Cannot abort the transaction without an ongoing transaction.\n");
            return;
        }
    }

    /*!
        Commits the current transaction.

        A fatal error will be raised if called when the engine is not in transaction state.
    */
    void Model::commitTransaction(const Variant &message) {
        Q_D(Model);
        if (d->state != Transaction) {
            SUBSTATE_FATAL("Cannot commit the transaction without an ongoing transaction.\n");
            return;
        }
        if (d->txActions.empty())
            return;

        d->engine->commit(d->txActions, message);
        d->txActions.clear();
    }

    void Model::undo() {
        Q_D(Model);
        if (d->state != Idle) {
            SUBSTATE_FATAL("Attempt to undo at an invalid state.\n");
            return;
        }
        d->state = Undo;
        d->engine->execute(true);
        d->state = Idle;
    }

    void Model::redo() {
        Q_D(Model);
        if (d->state != Idle) {
            SUBSTATE_FATAL("Attempt to redo at an invalid state.\n");
            return;
        }
        d->state = Redo;
        d->engine->execute(false);
        d->state = Idle;
    }

    bool Model::isWritable() const {
        Q_D(const Model);
        return d->state == Transaction && !d->lockedNode;
    }

    void Model::dispatch(Action *action, bool done) {
        Sender::dispatch(action, done);

        Q_D(Model);
        if (d->state == Transaction && done) {
            d->txActions.push_back(action);
        }
    }

    Model::Model(ModelPrivate &d) : Sender(d) {
        d.init();
    }

}