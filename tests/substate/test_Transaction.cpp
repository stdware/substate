#include <deque>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include <substate/Model.h>
#include <substate/StorageEngine.h>
#include <substate/Transaction.h>

#include <boost/test/unit_test.hpp>

#include "TestTree.h"

using namespace ss;

BOOST_AUTO_TEST_SUITE(test_Transaction)

namespace {

    // A storage engine without a step limit that gives access to its transactions.
    class KeepingEngine : public StorageEngine {
    public:
        std::deque<Transaction> transactions;
        int executed = 0;

        void commit(Transaction transaction) override {
            transactions.erase(transactions.begin() + executed, transactions.end());
            transactions.push_back(std::move(transaction));
            ++executed;
        }

        Transaction *previousTransaction() override {
            return executed == 0 ? nullptr : &transactions[size_t(--executed)];
        }

        Transaction *nextTransaction() override {
            return executed == int(transactions.size()) ? nullptr
                                                        : &transactions[size_t(executed++)];
        }

        void reset() override {
            transactions.clear();
            executed = 0;
        }

        int minimumStep() const override {
            return 0;
        }

        int maximumStep() const override {
            return int(transactions.size());
        }

        int currentStep() const override {
            return executed;
        }

        std::map<std::string, std::string> stepMessage(int step) const override {
            (void) step;
            return {};
        }
    };

    std::vector<Node *> heldNodes(const Transaction &transaction) {
        std::vector<Node *> nodes;
        transaction.forEachHeldNode([&nodes](Node *node) { nodes.push_back(node); });
        return nodes;
    }

}

// A transaction owns the nodes that its actions own in their present state: the removed nodes
// while it is executed, and the inserted nodes while it is undone. Descendants are not reported.
BOOST_AUTO_TEST_CASE(test_the_held_nodes_follow_the_state_of_the_actions) {
    auto engine = new KeepingEngine();
    Model model{std::unique_ptr<StorageEngine>(engine)};
    model.reset(makeNode(1));
    auto root = static_cast<CountingNode *>(model.root());
    auto removed = root->child(0);

    model.beginTransaction();
    root->append(makeNode(1));
    auto inserted = root->child(1);
    root->remove(0, 1);
    model.commitTransaction();

    const auto &transaction = engine->transactions.front();
    BOOST_CHECK(heldNodes(transaction) == std::vector<Node *>{removed});
    model.undo();
    BOOST_CHECK(heldNodes(transaction) == std::vector<Node *>{inserted});
    model.redo();
    BOOST_CHECK(heldNodes(transaction) == std::vector<Node *>{removed});
}

BOOST_AUTO_TEST_CASE(test_a_root_change_holds_the_root_outside_the_tree) {
    auto engine = new KeepingEngine();
    Model model{std::unique_ptr<StorageEngine>(engine)};
    model.reset(makeNode());
    auto oldRoot = model.root();

    auto replacement = makeNode();
    auto newRoot = replacement.get();
    model.beginTransaction();
    model.setRoot(std::move(replacement));
    model.commitTransaction();

    const auto &transaction = engine->transactions.front();
    BOOST_CHECK(heldNodes(transaction) == std::vector<Node *>{oldRoot});
    model.undo();
    BOOST_CHECK(heldNodes(transaction) == std::vector<Node *>{newRoot});
}

BOOST_AUTO_TEST_SUITE_END()
