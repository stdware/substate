#include "ModelNotifier.h"

#include <cassert>

#include <substate/Model.h>
#include <substate/ModelObserver.h>

namespace ss {

    // A private observer, so that the signals can have the names of the observer functions.
    class ModelNotifier::Observer : public ModelObserver {
    public:
        explicit Observer(ModelNotifier *notifier) : m_notifier(notifier) {
        }

        void actionAboutToApply(const Action &action, Action::Operation operation) override {
            Q_EMIT m_notifier->actionAboutToApply(&action, operation);
        }

        void actionApplied(const Action &action, Action::Operation operation) override {
            Q_EMIT m_notifier->actionApplied(&action, operation);
        }

        void stepChanged(int step) override {
            Q_EMIT m_notifier->stepChanged(step);
        }

        void nodeAboutToBeDestroyed(Node *node) override {
            Q_EMIT m_notifier->nodeAboutToBeDestroyed(node);
        }

        void aboutToReset() override {
            Q_EMIT m_notifier->aboutToReset();
        }

        void resetFinished() override {
            Q_EMIT m_notifier->resetFinished();
        }

    private:
        ModelNotifier *m_notifier;
    };

    ModelNotifier::ModelNotifier(Model *model, QObject *parent)
        : QObject(parent), m_model(model), m_observer(std::make_unique<Observer>(this)) {
        assert(model);
        m_model->addObserver(m_observer.get());
    }

    ModelNotifier::~ModelNotifier() {
        m_model->removeObserver(m_observer.get());
    }

}
