#include "Transaction.h"

#include "Node_p.h"

namespace ss {

    namespace {

        void aboutToDiscard(const std::vector<std::unique_ptr<Action>> &actions) {
            for (const auto &action : actions) {
                NodePrivate::aboutToDiscard(*action);
            }
        }

    }

    Transaction::Transaction(std::vector<std::unique_ptr<Action>> actions,
                             std::map<std::string, std::string> message)
        : m_actions(std::move(actions)), m_message(std::move(message)) {
    }

    Transaction::~Transaction() {
        aboutToDiscard(m_actions);
    }

    Transaction::Transaction(Transaction &&RHS) noexcept = default;

    Transaction &Transaction::operator=(Transaction &&RHS) noexcept {
        if (this != &RHS) {
            // The actions replaced by the assignment are discarded.
            aboutToDiscard(m_actions);
            m_actions = std::move(RHS.m_actions);
            RHS.m_actions.clear();
            m_message = std::move(RHS.m_message);
        }
        return *this;
    }

    void Transaction::forEachHeldNode(const std::function<void(Node *)> &func) const {
        for (const auto &action : m_actions) {
            action->forEachHeldNode(func);
        }
    }

}
