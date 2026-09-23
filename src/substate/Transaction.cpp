#include "Transaction.h"

namespace ss {

    Transaction::Transaction(std::vector<std::unique_ptr<Action>> actions,
                             std::map<std::string, std::string> message)
        : m_actions(std::move(actions)), m_message(std::move(message)) {
    }

    Transaction::~Transaction() = default;

    Transaction::Transaction(Transaction &&RHS) noexcept = default;

    Transaction &Transaction::operator=(Transaction &&RHS) noexcept = default;

    void Transaction::forEachHeldNode(const std::function<void(Node *)> &func) const {
        for (const auto &action : m_actions) {
            action->forEachHeldNode(func);
        }
    }

}
