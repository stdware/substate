#include "Action.h"

#include "Model_p.h"
#include "Node_p.h"

namespace ss {

    std::unique_ptr<Action> RootChangeAction::clone(bool detach) const {
        auto action = std::make_unique<RootChangeAction>(_oldRoot, _newRoot);
        if (detach) {
            action->_oldRoot = NodePrivate::clone(action->_oldRoot.get(), true);
            action->_newRoot = NodePrivate::clone(action->_newRoot.get(), true);
        }
        return action;
    }

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