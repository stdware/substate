#include "MemoryStorageEngine.h"

#include <cassert>

namespace ss {

    MemoryStorageEngine::MemoryStorageEngine(int stepLimit) : m_stepLimit(stepLimit) {
        assert(stepLimit > 0);
    }

    MemoryStorageEngine::~MemoryStorageEngine() = default;

    void MemoryStorageEngine::setStepLimit(int stepLimit) {
        assert(stepLimit > 0);
        m_stepLimit = stepLimit;
    }

    void MemoryStorageEngine::commit(Transaction transaction) {
        // Truncation of the undone transactions.
        m_transactions.erase(m_transactions.begin() + m_executed, m_transactions.end());

        m_transactions.push_back(std::move(transaction));
        ++m_executed;

        // Eviction of the oldest transactions, all of which are executed at this point.
        while (int(m_transactions.size()) > m_stepLimit) {
            m_transactions.pop_front();
            --m_executed;
            ++m_evicted;
        }
    }

    Transaction *MemoryStorageEngine::previousTransaction() {
        if (m_executed == 0) {
            return nullptr;
        }
        --m_executed;
        return &m_transactions[size_t(m_executed)];
    }

    Transaction *MemoryStorageEngine::nextTransaction() {
        if (m_executed == int(m_transactions.size())) {
            return nullptr;
        }
        return &m_transactions[size_t(m_executed++)];
    }

    void MemoryStorageEngine::reset() {
        m_transactions.clear();
        m_evicted = 0;
        m_executed = 0;
    }

    int MemoryStorageEngine::minimumStep() const {
        return m_evicted;
    }

    int MemoryStorageEngine::maximumStep() const {
        return m_evicted + int(m_transactions.size());
    }

    int MemoryStorageEngine::currentStep() const {
        return m_evicted + m_executed;
    }

    std::map<std::string, std::string> MemoryStorageEngine::stepMessage(int step) const {
        const int index = step - m_evicted - 1;
        if (index < 0 || index >= int(m_transactions.size())) {
            return {};
        }
        return m_transactions[size_t(index)].message();
    }

}
