// Copyright (C) 2022-2025 Stdware Collections (https://www.github.com/stdware)
// SPDX-License-Identifier: Apache-2.0

#ifndef SUBSTATE_MEMORYSTORAGEENGINE_H
#define SUBSTATE_MEMORYSTORAGEENGINE_H

#include <deque>

#include <substate/StorageEngine.h>

namespace ss {

    /// A storage engine that keeps the history in memory, with a limit on the number of
    /// transactions.
    class SUBSTATE_EXPORT MemoryStorageEngine : public StorageEngine {
    public:
        /// \param stepLimit the maximum number of transactions kept, which must be positive
        explicit MemoryStorageEngine(int stepLimit = 100);
        ~MemoryStorageEngine();

        inline int stepLimit() const;

        /// Sets the maximum number of transactions kept, which must be positive. The limit is
        /// applied at the next commit, when only executed transactions remain after truncation,
        /// by evicting the oldest.
        void setStepLimit(int stepLimit);

        void commit(Transaction transaction) override;
        Transaction *previousTransaction() override;
        Transaction *nextTransaction() override;
        void reset() override;

        int minimumStep() const override;
        int maximumStep() const override;
        int currentStep() const override;
        std::map<std::string, std::string> stepMessage(int step) const override;

    private:
        int m_stepLimit;

        // The steps of the transactions evicted so far.
        int m_evicted = 0;

        // The number of executed transactions in m_transactions.
        int m_executed = 0;

        std::deque<Transaction> m_transactions;
    };

    inline int MemoryStorageEngine::stepLimit() const {
        return m_stepLimit;
    }

}

#endif // SUBSTATE_MEMORYSTORAGEENGINE_H
