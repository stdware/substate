#ifndef SUBSTATE_TESTS_CODECTESTTOOLS_H
#define SUBSTATE_TESTS_CODECTESTTOOLS_H

#include <deque>
#include <map>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

#include <substate/Codec.h>
#include <substate/Model.h>
#include <substate/ModelObserver.h>
#include <substate/StorageEngine.h>
#include <substate/private/Node_p.h>

/// Returns the encoding of \a node and its descendants.
inline std::string encodeNode(const ss::Node *node) {
    std::ostringstream stream(std::ios::binary);
    ss::OBinaryStream out(stream);
    ss::Encoder encoder(out);
    encoder.writeNode(node);
    return encoder.fail() ? std::string() : stream.str();
}

/// Decodes the node of \a bytes into \a model, or as a free node if \a model is \c nullptr.
/// Returns \c nullptr on failure, and also if \a bytes contains more than the node.
inline std::unique_ptr<ss::Node> decodeNode(const ss::Codec &codec, const std::string &bytes,
                                            ss::Model *model) {
    std::istringstream stream(bytes, std::ios::binary);
    ss::IBinaryStream in(stream);
    ss::Decoder decoder(codec, in, model);
    auto node = decoder.readNode();
    if (decoder.fail() || stream.peek() != std::char_traits<char>::eof()) {
        return nullptr;
    }
    return node;
}

/// Decodes the action of \a bytes for \a model in \a state, taking the nodes that it owns from
/// \a pool if it is applied. Returns \c nullptr on failure, and also if \a bytes contains more
/// than the action.
inline std::unique_ptr<ss::Action> decodeAction(const ss::Codec &codec, const std::string &bytes,
                                                ss::Model *model, ss::NodePool *pool = nullptr,
                                                ss::Action::State state = ss::Action::Unapplied) {
    std::istringstream stream(bytes, std::ios::binary);
    ss::IBinaryStream in(stream);
    ss::Decoder decoder(codec, in, model, pool);
    auto action = decoder.readAction(state);
    if (decoder.fail() || stream.peek() != std::char_traits<char>::eof()) {
        return nullptr;
    }
    return action;
}

/// An observer that encodes every action at its first execution, as a storage engine with a
/// log does, and collects the encodings of the transaction in progress.
class ActionRecorder : public ss::ModelObserver {
public:
    /// The encodings of the actions since the caller cleared the list.
    std::vector<std::string> actions;

    /// Whether an encoding failed.
    bool failed = false;

    inline void actionApplied(const ss::Action &action, ss::Action::Operation operation) override {
        if (operation != ss::Action::Execute) {
            return;
        }
        std::ostringstream stream(std::ios::binary);
        ss::OBinaryStream out(stream);
        ss::Encoder encoder(out);
        encoder.writeAction(action);
        failed = failed || encoder.fail();
        actions.push_back(stream.str());
    }
};

/// Decodes \a actions for \a model and executes them as one transaction, as the recovery of a
/// storage engine does. Returns whether every action was decoded.
inline bool replay(ss::Model &model, const ss::Codec &codec,
                   const std::vector<std::string> &actions) {
    model.beginTransaction();
    for (const auto &bytes : actions) {
        auto action = decodeAction(codec, bytes, &model);
        if (!action) {
            model.abortTransaction();
            return false;
        }
        ss::NodePrivate::execute(&model, std::move(action));
    }
    model.commitTransaction();
    return true;
}

/// A storage engine with the truncation and eviction of MemoryStorageEngine that also keeps the
/// encodings of the actions of each transaction, as a log of a persistent storage engine does,
/// and can install a restored history.
class LoggingEngine : public ss::StorageEngine {
public:
    /// The encodings of the actions of the transaction committed next, which the caller sets.
    std::vector<std::string> pending;

    inline explicit LoggingEngine(int stepLimit) : m_stepLimit(stepLimit) {
    }

    inline void commit(ss::Transaction transaction) override {
        m_transactions.erase(m_transactions.begin() + m_executed, m_transactions.end());
        m_encodings.erase(m_encodings.begin() + m_executed, m_encodings.end());
        m_transactions.push_back(std::move(transaction));
        m_encodings.push_back(std::move(pending));
        pending.clear();
        ++m_executed;
        while (int(m_transactions.size()) > m_stepLimit) {
            m_transactions.pop_front();
            m_encodings.pop_front();
            --m_executed;
            ++m_evicted;
        }
    }

    inline ss::Transaction *stepBackward() override {
        return m_executed == 0 ? nullptr : &m_transactions[size_t(--m_executed)];
    }

    inline ss::Transaction *stepForward() override {
        return m_executed == int(m_transactions.size()) ? nullptr
                                                        : &m_transactions[size_t(m_executed++)];
    }

    inline void reset() override {
        m_transactions.clear();
        m_encodings.clear();
        m_executed = 0;
        m_evicted = 0;
    }

    inline int minimumStep() const override {
        return m_evicted;
    }

    inline int maximumStep() const override {
        return m_evicted + int(m_transactions.size());
    }

    inline int currentStep() const override {
        return m_evicted + m_executed;
    }

    inline std::map<std::string, std::string> stepMessage(int step) const override {
        (void) step;
        return {};
    }

    inline int stepLimit() const {
        return m_stepLimit;
    }

    inline int executed() const {
        return m_executed;
    }

    inline int evicted() const {
        return m_evicted;
    }

    inline const std::deque<ss::Transaction> &transactions() const {
        return m_transactions;
    }

    inline const std::deque<std::vector<std::string>> &encodings() const {
        return m_encodings;
    }

    /// Replaces the history with \a transactions, of which the first \a executed are applied.
    inline void install(std::deque<ss::Transaction> transactions,
                        std::deque<std::vector<std::string>> encodings, int executed, int evicted) {
        m_transactions = std::move(transactions);
        m_encodings = std::move(encodings);
        m_executed = executed;
        m_evicted = evicted;
    }

private:
    int m_stepLimit;
    std::deque<ss::Transaction> m_transactions;
    std::deque<std::vector<std::string>> m_encodings;
    int m_executed = 0;
    int m_evicted = 0;
};

/// Restores the tree and the retained history of \a source, whose storage engine is \a engine,
/// into a new model, as a persistent storage engine does from a checkpoint and a log. Returns
/// \c nullptr on failure.
///
/// The checkpoint consists of the tree and of the nodes that the applied actions own. The
/// actions are decoded in the state they have in \a source, and the applied actions take their
/// nodes from a NodePool, which must be empty afterwards.
inline std::unique_ptr<ss::Model> restoreCopy(const ss::Codec &codec, const ss::Model &source,
                                              const LoggingEngine &engine) {
    auto targetEngine = new LoggingEngine(engine.stepLimit());
    auto target = std::make_unique<ss::Model>(std::unique_ptr<ss::StorageEngine>(targetEngine));

    // The checkpoint.
    const auto tree = encodeNode(source.root());
    std::vector<std::string> owned;
    for (int i = 0; i < engine.executed(); ++i) {
        engine.transactions()[size_t(i)].forEachHeldNode(
            [&owned](ss::Node *node) { owned.push_back(encodeNode(node)); });
    }

    auto root = decodeNode(codec, tree, target.get());
    if (source.root() && !root) {
        return nullptr;
    }
    ss::NodePool pool;
    for (const auto &bytes : owned) {
        if (!pool.add(decodeNode(codec, bytes, target.get()))) {
            return nullptr;
        }
    }

    // The log.
    std::deque<ss::Transaction> transactions;
    for (size_t i = 0; i < engine.encodings().size(); ++i) {
        const auto state = int(i) < engine.executed() ? ss::Action::Applied : ss::Action::Unapplied;
        std::vector<std::unique_ptr<ss::Action>> actions;
        for (const auto &bytes : engine.encodings()[i]) {
            auto action = decodeAction(codec, bytes, target.get(), &pool, state);
            if (!action) {
                return nullptr;
            }
            actions.push_back(std::move(action));
        }
        transactions.emplace_back(std::move(actions), std::map<std::string, std::string>());
    }
    if (!pool.empty()) {
        return nullptr;
    }

    targetEngine->install(std::move(transactions), engine.encodings(), engine.executed(),
                          engine.evicted());
    target->restore(std::move(root));
    return target;
}

/// Undoes \a first and \a second to their oldest step and redoes them to their newest step,
/// comparing them with \a same after every step, and then undoes them back to the step they had.
/// Returns whether \a same held throughout.
template <class Same>
inline bool sweepHistory(ss::Model &first, ss::Model &second, Same same) {
    const int step = first.currentStep();
    bool equal = same();
    while (equal && first.canUndo()) {
        first.undo();
        second.undo();
        equal = same();
    }
    while (equal && first.canRedo()) {
        first.redo();
        second.redo();
        equal = same();
    }
    while (first.currentStep() > step) {
        first.undo();
        second.undo();
    }
    while (first.currentStep() < step) {
        first.redo();
        second.redo();
    }
    return equal && same();
}

#endif // SUBSTATE_TESTS_CODECTESTTOOLS_H
