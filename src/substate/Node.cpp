#include "Node.h"

#include <cassert>

#include "Node_p.h"
#include "Model_p.h"
#include "Model.h"
#include "StorageEngine.h"
#include "Stream.h"

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

    void Node::write(Node &node, std::ostream &os) {
        OStream stream(&os);
        stream << node.type() << node.classType();
        node.write(os);
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

    NodeAction::~NodeAction() = default;

    void NodeAction::write(std::ostream &os) const {
        OStream stream(&os);
        const auto &parent = _parent.value();
        stream << (parent ? parent->id() : 0);
    }

    void NodeAction::read(std::istream &is) {
        IStream stream(&is);

        size_t parentIndex;
        stream >> parentIndex;

        _parent.temp() = parentIndex;
    }

    void NodeAction::initialize(const std::function<std::shared_ptr<Node>(size_t /*id*/)> &find) {
        size_t parentIndex = _parent.temp();
        _parent.load(parentIndex == 0 ? std::shared_ptr<Node>() : find(parentIndex));
    }

    RootChangeAction::~RootChangeAction() = default;

    std::unique_ptr<Action> RootChangeAction::clone(bool detach) const {
        auto action = std::make_unique<RootChangeAction>(_oldRoot, _newRoot);
        if (detach) {
            action->_oldRoot = NodePrivate::clone(action->_oldRoot->get(), true);
            action->_newRoot = NodePrivate::clone(action->_newRoot->get(), true);
        }
        return action;
    }

    void RootChangeAction::write(std::ostream &os) const {
        OStream stream(&os);
        const auto &oldRoot = _oldRoot.value();
        const auto &newRoot = _newRoot.value();
        stream << (oldRoot ? oldRoot->id() : 0) << (newRoot ? newRoot->id() : 0);
    }

    void RootChangeAction::queryNodes(
        bool inserted, const std::function<void(const std::shared_ptr<Node> &)> &add) {
        if (inserted) {
            if (_newRoot.value()) {
                add(_newRoot);
            }
        } else {
            if (_oldRoot.value()) {
                add(_oldRoot);
            }
        }
    }

    void RootChangeAction::execute(bool undo) {
        const auto &oldRoot = _oldRoot.value();
        const auto &newRoot = _newRoot.value();
        ModelPrivate::setRoot(newRoot ? newRoot->model() : oldRoot->model(),
                              undo ? oldRoot : newRoot);
    }

    void RootChangeAction::read(std::istream &is) {
        IStream stream(&is);

        size_t oldRootIndex, newRootIndex;
        stream >> oldRootIndex >> newRootIndex;

        _oldRoot.temp() = oldRootIndex;
        _newRoot.temp() = newRootIndex;
    }

    void RootChangeAction::initialize(
        const std::function<std::shared_ptr<Node>(size_t /*id*/)> &find) {
        size_t oldId = _oldRoot.temp();
        size_t newId = _newRoot.temp();
        _oldRoot.load(oldId == 0 ? std::shared_ptr<Node>() : find(oldId));
        _newRoot.load(newId == 0 ? std::shared_ptr<Node>() : find(newId));
    }

    std::shared_ptr<Node> NodeReader::readNode(std::istream &is) {
        IStream stream(&is);
        int type, classType;
        stream >> type >> classType;
        return readNode(type, classType, is);
    }

}