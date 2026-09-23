#include "Model.h"

#include <cassert>

#include "MemoryStorageEngine.h"
#include "Node_p.h"

namespace ss {

    Model::Model() : Model(std::make_unique<MemoryStorageEngine>()) {
    }

    Model::Model(std::unique_ptr<StorageEngine> storageEngine)
        : m_storageEngine(std::move(storageEngine)) {
        assert(m_storageEngine);
    }

    Model::~Model() {
        assert(m_state == State::Idle || m_state == State::Transaction);

        // The order required by docs/Design.md, stated explicitly rather than left to the
        // destruction of the members: the transaction in progress and the history, then the
        // tree. Every node removes itself from the index when destroyed.
        m_actions.clear();
        m_storageEngine.reset();
        m_root.reset();
        assert(m_index.empty());
    }

    Node *Model::nodeById(std::uint64_t id) const {
        auto it = m_index.find(id);
        return it == m_index.end() ? nullptr : it->second;
    }

    void Model::setRoot(std::unique_ptr<Node> root) {
        assert(inTransaction());
        assert(!root || NodePrivate::isInsertable(root.get()));

        std::unique_ptr<RootChangeAction> action(new RootChangeAction(this, std::move(root)));
        action->execute(Action::Execute);
        m_actions.push_back(std::move(action));
    }

    void Model::reset(std::unique_ptr<Node> root) {
        assert(m_state == State::Idle);
        assert(!root || NodePrivate::isInsertable(root.get()));

        m_storageEngine->reset();
        m_root.reset();
        assert(m_index.empty());

        if (root) {
            m_root = std::move(root);
            NodePrivate::attach(m_root.get(), nullptr, this);
        }
    }

    void Model::beginTransaction() {
        assert(m_state == State::Idle);
        m_state = State::Transaction;
    }

    void Model::abortTransaction() {
        assert(m_state == State::Transaction);

        for (auto it = m_actions.rbegin(); it != m_actions.rend(); ++it) {
            (*it)->execute(Action::Undo);
        }
        m_actions.clear();
        m_state = State::Idle;
    }

    void Model::commitTransaction(std::map<std::string, std::string> message) {
        assert(m_state == State::Transaction);
        m_state = State::Idle;

        if (m_actions.empty()) {
            return;
        }

        Transaction transaction(std::move(m_actions), std::move(message));
        m_actions.clear();
        m_storageEngine->commit(std::move(transaction));
    }

    bool Model::canUndo() const {
        return currentStep() > minimumStep();
    }

    bool Model::canRedo() const {
        return currentStep() < maximumStep();
    }

    void Model::undo() {
        assert(m_state == State::Idle);

        auto transaction = m_storageEngine->stepBackward();
        assert(transaction);
        if (!transaction) {
            return;
        }

        m_state = State::Undo;
        const auto &actions = transaction->actions();
        for (auto it = actions.rbegin(); it != actions.rend(); ++it) {
            (*it)->execute(Action::Undo);
        }
        m_state = State::Idle;
    }

    void Model::redo() {
        assert(m_state == State::Idle);

        auto transaction = m_storageEngine->stepForward();
        assert(transaction);
        if (!transaction) {
            return;
        }

        m_state = State::Redo;
        for (const auto &action : transaction->actions()) {
            action->execute(Action::Redo);
        }
        m_state = State::Idle;
    }

    int Model::minimumStep() const {
        return m_storageEngine->minimumStep();
    }

    int Model::maximumStep() const {
        return m_storageEngine->maximumStep();
    }

    int Model::currentStep() const {
        return m_storageEngine->currentStep();
    }

    std::map<std::string, std::string> Model::stepMessage(int step) const {
        return m_storageEngine->stepMessage(step);
    }

}
