#include "Action.h"

#include "Model_p.h"

namespace ss {

    void RootChangeAction::queryNodes(
        bool inserted, const std::function<void(const std::shared_ptr<Node> &)> &add) {
        if (inserted) {
            if (_newRoot) {
                add(_newRoot);
            }
        } else {
            if (_oldRoot) {
                add(_oldRoot);
            }
        }
    }

    void RootChangeAction::execute(bool undo) {
        ModelPrivate::setRoot(_newRoot ? _newRoot->model() : _oldRoot->model(),
                              undo ? _oldRoot : _newRoot);
    }

}