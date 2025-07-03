#include "Model.h"

#include <cassert>
#include <utility>

#include "StorageEngine.h"
#include "Node_p.h"
#include "Model_p.h"

namespace ss {

    void ModelPrivate::setRoot_TX(Model *model, const std::shared_ptr<Node> &node) {
        auto &root = model->_root;
        model->_lockedNode = root ? root.get() : node.get();

        RootChangeAction a(node, root);

        // Pre-Propagate
        {
            ActionNotification n(Notification::ActionAboutToTrigger, &a);
            model->notify(&n);
        }

        // Do change
        if (root) {
            root->_state = Node::Detached;
        }
        if (node) {
            node->_state = Node::Active;
        }
        root = node;

        // Propagate signal
        {
            ActionNotification n(Notification::ActionTriggered, &a);
            model->notify(&n);
        }

        model->_lockedNode = nullptr;
    }

    Model::Model(std::unique_ptr<StorageEngine> storageEngine)
        : _storageEngine(std::move(storageEngine)) {
        _storageEngine->setup(this);
    }

    Model::~Model() {
        // do what?
    }

    bool Model::isWritable() const {
        return _state == Transaction && !_lockedNode;
    }

    std::shared_ptr<Node> Model::indexOf(size_t id) const {
        return _storageEngine->indexOf(id);
    }

    void Model::setRoot(const std::shared_ptr<Node> &node) {
        assert(isWritable());
        assert(!node || node->isFree());
        ModelPrivate::setRoot_TX(this, node);
    }

    void Model::reset() {
        assert(_state == Idle);

        Notification n(Notification::AboutToReset);
        notify(&n);
        _storageEngine->reset();
    }

    void Model::beginTransaction() {
        assert(_state == Idle);

        _state = Transaction;
        _storageEngine->prepare();
    }

    void Model::abortTransaction() {
        assert(_state == Transaction);

        auto &stack = _txActions;
        for (auto it = stack.rbegin(); it != stack.rend(); ++it) {
            const auto &a = *it;
            a->execute(true);
        }
        stack.clear();

        _storageEngine->abort();
        _state = Idle;
    }

    void Model::commitTransaction(std::map<std::string, std::string> message) {
        assert(_state == Transaction);
        if (_txActions.empty()) {
            _state = Idle;
            return;
        }

        // Associate nodes with model
        std::vector<std::shared_ptr<Node>> nodes;
        for (auto &a : std::as_const(_txActions)) {
            a->queryNodes(true, [&nodes](const std::shared_ptr<Node> &node) {
                nodes.push_back(node); //
            });
        }
        for (const auto &node : std::as_const(nodes)) {
            node->propagate([this](Node *node) { NodePrivate::propagate(node, this); });
        }

        // Commit transaction to storage engine
        {
            std::vector<std::unique_ptr<Action>> actions;
            actions.swap(_txActions);
            _storageEngine->commit(std::move(actions), std::move(message));
        }

        _state = Idle;

        Notification n(Notification::StepChange);
        notify(&n);
    }

    std::map<std::string, std::string> Model::stepMessage(int step) const {
        return _storageEngine->stepMessage(step);
    }

    void Model::undo() {
        assert(_state == Idle);

        _state = Undo;
        _storageEngine->execute(true);
        _state = Idle;

        Notification n(Notification::StepChange);
        notify(&n);
    }

    void Model::redo() {
        assert(_state == Idle);
        _state = Redo;
        _storageEngine->execute(false);
        _state = Idle;

        Notification n(Notification::StepChange);
        notify(&n);
    }

    int Model::minimumStep() const {
        return _storageEngine->minimum();
    }

    int Model::maximumStep() const {
        return _storageEngine->maximum();
    }

    int Model::currentStep() const {
        return _storageEngine->current();
    }

    void Model::notify(Notification *n) {
    }

}