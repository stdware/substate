#include "Node.h"

#include <cassert>

#include "Node_p.h"
#include "Model.h"
#include "StorageEngine.h"

namespace ss {

    void NodePrivate::propagate(Node *node, Model *model) {
        auto engine = model->storageEngine();
        node->propagate([model, engine](Node *node) {
            node->_model = model;
            node->_id = engine->addId(node, node->_id);
        });
    }

    Node::~Node() {
        if (_id > 0) {
            assert(_model);
            if (!_model->_clearing)
                _model->_storageEngine->removeId(_id);
        }
    }

    bool Node::isDetached() const {
        if (isFree())
            return false;

        // The node is removed?
        if (_state == Detached)
            return true;

        // The parent is obsolete?
        if (auto parent = _parent)
            return parent->isDetached();

        return false;
    }

    bool Node::isWritable() const {
        // The node is not managed by a model?
        if (!_model)
            return true;

        // The parent is writable?
        if (auto parent = _parent)
            return parent->isWritable();

        return _model->isWritable() && _state != Detached;
    }

    void Node::addChild(Node *node) {
        node->_parent = this;
        node->_state = Active;
    }

    void Node::removeChild(Node *node) {
        node->_parent = nullptr;
        if (_model)
            node->_state = Detached;
    }

    void Node::beginAction() {
        if (_model)
            _model->_lockedNode = this;
    }

    void Node::endAction() {
        if (_model)
            _model->_lockedNode = nullptr;
    }

    void Node::propagateChildren(const std::function<void(Node *)> &func) {
        (void) func;
    }

    void Node::notified(Notification *n) {
        switch (n->type()) {
            case Notification::ActionAboutToTrigger:
            case Notification::ActionTriggered: {
                if (_model) {
                    _model->notified(n);
                }
                break;
            }
            default:
                break;
        }
    }

}