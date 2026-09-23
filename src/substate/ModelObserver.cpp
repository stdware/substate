#include "ModelObserver.h"

namespace ss {

    ModelObserver::ModelObserver() = default;

    ModelObserver::~ModelObserver() = default;

    void ModelObserver::actionAboutToApply(const Action &action, Action::Operation operation) {
        (void) action;
        (void) operation;
    }

    void ModelObserver::actionApplied(const Action &action, Action::Operation operation) {
        (void) action;
        (void) operation;
    }

    void ModelObserver::stepChanged(int step) {
        (void) step;
    }

    void ModelObserver::nodeAboutToBeDestroyed(Node *node) {
        (void) node;
    }

    void ModelObserver::aboutToReset() {
    }

    void ModelObserver::resetFinished() {
    }

}
