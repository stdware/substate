#include "model.h"
#include "model_p.h"

namespace Substate {

    ModelPrivate::ModelPrivate() {
        state = Model::Idle;
    }

    ModelPrivate::~ModelPrivate() {
        for (const auto &sub : std::as_const(subscribers))
            sub->m_model = nullptr;
    }

    void ModelPrivate::init() {
    }

    Model::Model() : Model(*new ModelPrivate()) {
    }

    Model::~Model() {
    }

    Model::State Model::state() const {
        Q_D(const Model);
        return d->state;
    }

    void Model::beginTransaction() {
    }

    void Model::abortTransaction() {
    }

    void Model::commitTransaction() {
    }

    void Model::addSubscriber(Subscriber *sub) {
        Q_D(Model);
        auto it = d->subscriberIndexes.find(sub);
        if (it != d->subscriberIndexes.end())
            return;
        d->subscriberIndexes.insert(
            std::make_pair(sub, d->subscribers.insert(d->subscribers.end(), sub)));

        sub->m_model = this;
    }

    void Model::removeSubscriber(Subscriber *sub) {
        Q_D(Model);
        auto it = d->subscriberIndexes.find(sub);
        if (it == d->subscriberIndexes.end())
            return;
        d->subscribers.erase(it->second);
        d->subscriberIndexes.erase(it);

        sub->m_model = nullptr;
    }

    void Model::dispatch(Operation *op, bool done) {
        Q_D(Model);

        // Notify
        for (const auto &sub : std::as_const(d->subscribers)) {
            sub->operation(op, done);
        }

        if (done) {
            // TODO: Commit to backend
        }
    }

    Model::Model(ModelPrivate &d) : d_ptr(&d) {
        d.q_ptr = this;
        d.init();
    }

    Subscriber::Subscriber() : m_model(nullptr) {
    }

    Subscriber::~Subscriber() {
        if (m_model)
            m_model->removeSubscriber(this);
    }

}