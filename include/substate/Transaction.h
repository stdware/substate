// Copyright (C) 2022-2025 Stdware Collections (https://www.github.com/stdware)
// SPDX-License-Identifier: Apache-2.0

#ifndef SUBSTATE_TRANSACTION_H
#define SUBSTATE_TRANSACTION_H

#include <map>
#include <memory>
#include <string>
#include <vector>

#include <substate/Action.h>

namespace ss {

    /// The actions of one committed transaction, in execution order, together with its message.
    /// A transaction is one undo step.
    ///
    /// Destroying a transaction, or replacing it by move assignment, destroys the nodes that its
    /// actions own, and reports each of them to ModelObserver::nodeAboutToBeDestroyed() first.
    class SUBSTATE_EXPORT Transaction {
    public:
        Transaction(std::vector<std::unique_ptr<Action>> actions,
                    std::map<std::string, std::string> message);
        ~Transaction();

        Transaction(Transaction &&RHS) noexcept;
        Transaction &operator=(Transaction &&RHS) noexcept;

        inline const std::vector<std::unique_ptr<Action>> &actions() const;
        inline const std::map<std::string, std::string> &message() const;

        /// Calls \a func on each node that an action of this transaction owns at present.
        void forEachHeldNode(const std::function<void(Node *)> &func) const;

    private:
        std::vector<std::unique_ptr<Action>> m_actions;
        std::map<std::string, std::string> m_message;
    };

    inline const std::vector<std::unique_ptr<Action>> &Transaction::actions() const {
        return m_actions;
    }

    inline const std::map<std::string, std::string> &Transaction::message() const {
        return m_message;
    }

}

#endif // SUBSTATE_TRANSACTION_H
