#include "Model.h"

#include <algorithm>
#include <cassert>

#include "MemoryStorageEngine.h"
#include "ModelObserver.h"
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
        assert(!m_notifying);

        notify([](ModelObserver *observer) { observer->aboutToReset(); });
        m_state = State::Reset;

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

        NodePrivate::execute(this, std::unique_ptr<RootChangeAction>(
                                       new RootChangeAction(this, std::move(root), m_root.get())));
    }

    void Model::reset(std::unique_ptr<Node> root) {
        assert(m_state == State::Idle && !m_notifying);
        assert(!root || NodePrivate::isInsertable(root.get()));

        notify([](ModelObserver *observer) { observer->aboutToReset(); });
        m_state = State::Reset;
        m_storageEngine->reset();
        m_root.reset();
        assert(m_index.empty());
        m_state = State::Idle;

        if (root) {
            m_root = std::move(root);
            NodePrivate::attach(m_root.get(), nullptr, this);
        }
        notify([](ModelObserver *observer) { observer->resetFinished(); });
    }

    void Model::restore(std::unique_ptr<Node> root, std::uint64_t lastId) {
        assert(m_state == State::Idle && !m_notifying);
        assert(!m_root);
        assert(!root || (root->model() == this && !root->parent() && !root->isAttached()));

        notify([](ModelObserver *observer) { observer->aboutToReset(); });
        m_lastId = std::max(m_lastId, lastId);
        if (root) {
            m_root = std::move(root);
            NodePrivate::attach(m_root.get(), nullptr, this);
        }
        notify([](ModelObserver *observer) { observer->resetFinished(); });
    }

    void Model::beginTransaction() {
        assert(m_state == State::Idle && !m_notifying);
        m_state = State::Transaction;
    }

    void Model::abortTransaction() {
        assert(m_state == State::Transaction && !m_notifying);

        for (auto it = m_actions.rbegin(); it != m_actions.rend(); ++it) {
            apply(**it, Action::Undo);
        }
        for (const auto &action : m_actions) {
            NodePrivate::aboutToDiscard(*action);
        }
        m_actions.clear();
        m_state = State::Idle;
    }

    void Model::commitTransaction(std::map<std::string, std::string> message) {
        assert(m_state == State::Transaction && !m_notifying);
        m_state = State::Idle;

        if (m_actions.empty()) {
            return;
        }

        Transaction transaction(std::move(m_actions), std::move(message));
        m_actions.clear();
        m_storageEngine->commit(std::move(transaction));

        const int step = currentStep();
        notify([step](ModelObserver *observer) { observer->stepChanged(step); });
    }

    bool Model::canUndo() const {
        return currentStep() > minimumStep();
    }

    bool Model::canRedo() const {
        return currentStep() < maximumStep();
    }

    void Model::undo() {
        assert(m_state == State::Idle && !m_notifying);

        auto transaction = m_storageEngine->stepBackward();
        assert(transaction);
        if (!transaction) {
            return;
        }

        m_state = State::Undo;
        const auto &actions = transaction->actions();
        for (auto it = actions.rbegin(); it != actions.rend(); ++it) {
            apply(**it, Action::Undo);
        }
        m_state = State::Idle;

        const int step = currentStep();
        notify([step](ModelObserver *observer) { observer->stepChanged(step); });
    }

    void Model::redo() {
        assert(m_state == State::Idle && !m_notifying);

        auto transaction = m_storageEngine->stepForward();
        assert(transaction);
        if (!transaction) {
            return;
        }

        m_state = State::Redo;
        for (const auto &action : transaction->actions()) {
            apply(*action, Action::Redo);
        }
        m_state = State::Idle;

        const int step = currentStep();
        notify([step](ModelObserver *observer) { observer->stepChanged(step); });
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

    void Model::addObserver(ModelObserver *observer) {
        assert(observer && !m_notifying);
        assert(std::find(m_observers.begin(), m_observers.end(), observer) == m_observers.end());
        m_observers.push_back(observer);
    }

    void Model::removeObserver(ModelObserver *observer) {
        assert(!m_notifying);
        m_observers.erase(std::remove(m_observers.begin(), m_observers.end(), observer),
                          m_observers.end());
    }

    void Model::apply(Action &action, Action::Operation operation) {
        // A modification from a notification function.
        assert(!m_notifying);

        notify([&](ModelObserver *observer) { observer->actionAboutToApply(action, operation); });
        action.execute(operation);
        notify([&](ModelObserver *observer) { observer->actionApplied(action, operation); });
    }

    void Model::aboutToDestroy(Node *node) {
        if (m_state == State::Reset || m_observers.empty()) {
            return;
        }
        NodePrivate::forEachInSubtree(node, [this](Node *n) {
            notify([n](ModelObserver *observer) { observer->nodeAboutToBeDestroyed(n); });
        });
    }

    void Model::notify(const std::function<void(ModelObserver *)> &func) {
        if (m_observers.empty()) {
            return;
        }
        m_notifying = true;
        for (auto observer : m_observers) {
            func(observer);
        }
        m_notifying = false;
    }

}
