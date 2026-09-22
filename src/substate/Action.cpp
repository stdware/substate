#include "Action.h"

#include "Model_p.h"

namespace ss {

    void RootChangeAction::queryNodes(bool inserted,
                                      const std::function<void(const NodePtr &)> &add) {
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
        Model *model;
        if (_newRoot) {
            model = _newRoot->model();
        } else {
            model = _oldRoot->model();
        }

        auto &oldRoot = model->_root;

        model->_lockedNode = oldRoot ? oldRoot.get() : node.get();

        // Pre-Propagate
        {
            if (undo) {
                // TODO
            }
            ActionNotification n(Notification::ActionAboutToTrigger, this);
            model->notify(&n);
        }

        // Do change
        if (root) {
            root->_state = Node::Detached;
        }
        if (node) {
            node->_state = Node::Active;
        }
        root = std::move(node);

        // Propagate signal
        {
            if (undo) {
                // TODO
            }
            ActionNotification n(Notification::ActionTriggered, &a);
            model->notify(&n);
        }

        model->_lockedNode = nullptr;

        ModelPrivate::setRoot(_newRoot ? _newRoot->model() : _oldRoot->model(),
                              undo ? _oldRoot : _newRoot);
    }

}
