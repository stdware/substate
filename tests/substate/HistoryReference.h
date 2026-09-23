#ifndef SUBSTATE_TESTS_HISTORYREFERENCE_H
#define SUBSTATE_TESTS_HISTORYREFERENCE_H

#include <cctype>
#include <cstdint>
#include <deque>
#include <set>
#include <string>
#include <vector>

/// Adds the identifiers in a configuration produced by dump() to \a ids. A number preceded by
/// \c # is the key of a SheetNode child, not an identifier.
inline void collectIds(const std::string &configuration, std::set<std::uint64_t> &ids) {
    std::uint64_t value = 0;
    bool inNumber = false;
    bool isKey = false;
    char previous = 0;
    for (char c : configuration) {
        if (std::isdigit(static_cast<unsigned char>(c))) {
            if (!inNumber) {
                isKey = previous == '#';
            }
            value = value * 10 + std::uint64_t(c - '0');
            inNumber = true;
        } else if (inNumber) {
            if (!isKey) {
                ids.insert(value);
            }
            value = 0;
            inNumber = false;
        }
        previous = c;
    }
    if (inNumber && !isKey) {
        ids.insert(value);
    }
}

/// The reference implementation of the history for the random test. It stores the
/// configuration of the tree after every action, as recorded when the action was first
/// executed, and applies the same truncation and eviction as MemoryStorageEngine.
///
/// By theorems 1 to 4 of docs/Design.md, the live nodes are exactly the nodes that occur in
/// the configuration of some retained position. The positions are the end of the oldest
/// retained step and every position after an action of a retained transaction.
class Reference {
public:
    explicit Reference(int stepLimit, std::string base)
        : m_stepLimit(stepLimit), m_base(std::move(base)) {
    }

    void record(std::string configuration) {
        m_pending.push_back(std::move(configuration));
    }

    void abort() {
        m_pending.clear();
    }

    void commit() {
        m_transactions.erase(m_transactions.begin() + m_executed, m_transactions.end());
        m_transactions.push_back(std::move(m_pending));
        m_pending.clear();
        ++m_executed;
        while (int(m_transactions.size()) > m_stepLimit) {
            m_base = m_transactions.front().back();
            m_transactions.pop_front();
            --m_executed;
        }
    }

    void undo() {
        --m_executed;
    }

    void redo() {
        ++m_executed;
    }

    /// The configuration at the current step.
    const std::string &current() const {
        return m_executed == 0 ? m_base : m_transactions[size_t(m_executed - 1)].back();
    }

    /// The configuration after the last action, within the transaction in progress if any.
    const std::string &latest() const {
        return m_pending.empty() ? current() : m_pending.back();
    }

    int executed() const {
        return m_executed;
    }

    int retained() const {
        return int(m_transactions.size());
    }

    std::set<std::uint64_t> liveIds() const {
        std::set<std::uint64_t> ids;
        collectIds(m_base, ids);
        for (const auto &transaction : m_transactions) {
            for (const auto &configuration : transaction) {
                collectIds(configuration, ids);
            }
        }
        for (const auto &configuration : m_pending) {
            collectIds(configuration, ids);
        }
        return ids;
    }

private:
    int m_stepLimit;
    std::string m_base;
    std::deque<std::vector<std::string>> m_transactions;
    std::vector<std::string> m_pending;
    int m_executed = 0;
};

#endif // SUBSTATE_TESTS_HISTORYREFERENCE_H
