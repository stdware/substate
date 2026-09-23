#include <memory>
#include <string>
#include <vector>

#include <substate/MemoryStorageEngine.h>
#include <substate/Model.h>
#include <substate/ModelObserver.h>

#include <boost/test/unit_test.hpp>

#include "TestTree.h"

using namespace ss;

BOOST_AUTO_TEST_SUITE(test_ModelObserver)

namespace {

    const char *operationName(Action::Operation operation) {
        switch (operation) {
            case Action::Execute:
                return "execute";
            case Action::Undo:
                return "undo";
            default:
                break;
        }
        return "redo";
    }

    /// Records every notification as a line of text.
    class RecordingObserver : public ModelObserver {
    public:
        std::vector<std::string> log;

        void actionAboutToApply(const Action &action, Action::Operation operation) override {
            log.push_back("about " + describe(action, operation));
        }

        void actionApplied(const Action &action, Action::Operation operation) override {
            log.push_back("applied " + describe(action, operation));
        }

        void stepChanged(int step) override {
            log.push_back("step " + std::to_string(step));
        }

        void nodeAboutToBeDestroyed(Node *node) override {
            log.push_back("destroy " + std::to_string(node->id()));
        }

        void aboutToReset() override {
            log.push_back("reset");
        }

        void resetFinished() override {
            log.push_back("reset finished");
        }

    private:
        // The operation and, for a VectorInsDelAction, the change for that operation and the
        // identifiers of the children, which are 0 before a first insertion.
        static std::string describe(const Action &action, Action::Operation operation) {
            std::string out = operationName(operation);
            if (action.type() == Action::VectorInsert || action.type() == Action::VectorRemove) {
                const auto &a = static_cast<const VectorInsDelAction &>(action);
                out += a.isInsertion(operation) ? " insert" : " remove";
                for (auto child : a.children()) {
                    out += ' ' + std::to_string(child->id());
                }
            }
            return out;
        }
    };

    std::unique_ptr<Model> makeModel(int stepLimit, std::unique_ptr<Node> root) {
        auto model = std::make_unique<Model>(std::make_unique<MemoryStorageEngine>(stepLimit));
        model->reset(std::move(root));
        return model;
    }

    CountingNode *rootOf(const Model &model) {
        return static_cast<CountingNode *>(model.root());
    }

    using Log = std::vector<std::string>;

}

BOOST_AUTO_TEST_CASE(test_execution_undo_and_redo_report_the_actual_change) {
    auto model = makeModel(10, makeNode());
    RecordingObserver observer;
    model->addObserver(&observer);
    const auto next = std::to_string(model->root()->id() + 1);

    model->beginTransaction();
    rootOf(*model)->append(makeNode());
    model->commitTransaction();
    model->undo();
    model->redo();

    // The inserted node receives its identifier when the insertion is applied.
    const Log expected{
        "about execute insert 0",    "applied execute insert " + next, "step 1",
        "about undo remove " + next, "applied undo remove " + next,    "step 0",
        "about redo insert " + next, "applied redo insert " + next,    "step 1",
    };
    BOOST_CHECK_EQUAL_COLLECTIONS(observer.log.begin(), observer.log.end(), expected.begin(),
                                  expected.end());
    model->removeObserver(&observer);
}

BOOST_AUTO_TEST_CASE(test_an_abort_reports_the_undo_and_then_the_destruction) {
    auto model = makeModel(10, makeNode());
    RecordingObserver observer;
    model->addObserver(&observer);

    model->beginTransaction();
    rootOf(*model)->append(makeNode(1));
    const auto parent = std::to_string(rootOf(*model)->child(0)->id());
    const auto child = std::to_string(rootOf(*model)->child(0)->child(0)->id());
    observer.log.clear();
    model->abortTransaction();

    // No step change, and the parent is reported before its child.
    const Log expected{
        "about undo remove " + parent,
        "applied undo remove " + parent,
        "destroy " + parent,
        "destroy " + child,
    };
    BOOST_CHECK_EQUAL_COLLECTIONS(observer.log.begin(), observer.log.end(), expected.begin(),
                                  expected.end());
    model->removeObserver(&observer);
}

BOOST_AUTO_TEST_CASE(test_truncation_and_eviction_report_the_destruction) {
    auto model = makeModel(1, makeNode(2));
    auto root = rootOf(*model);
    const auto first = std::to_string(root->child(0)->id());
    RecordingObserver observer;
    model->addObserver(&observer);

    // Evicting the removal destroys the removed node.
    model->beginTransaction();
    root->remove(0, 1);
    model->commitTransaction();
    model->beginTransaction();
    root->append(makeNode());
    model->commitTransaction();
    BOOST_CHECK_EQUAL(observer.log.at(observer.log.size() - 2), "destroy " + first);
    BOOST_CHECK_EQUAL(observer.log.back(), "step 2");

    // Truncating the undone insertion destroys the inserted node.
    const auto appended = std::to_string(root->child(1)->id());
    model->undo();
    const auto removed = std::to_string(root->child(0)->id());
    observer.log.clear();
    model->beginTransaction();
    root->remove(0, 1);
    model->commitTransaction();
    const Log expected{
        "about execute remove " + removed,
        "applied execute remove " + removed,
        "destroy " + appended,
        "step 2",
    };
    BOOST_CHECK_EQUAL_COLLECTIONS(observer.log.begin(), observer.log.end(), expected.begin(),
                                  expected.end());
    model->removeObserver(&observer);
}

BOOST_AUTO_TEST_CASE(test_reset_reports_no_destroyed_node) {
    auto model = makeModel(10, makeNode(1));
    model->beginTransaction();
    rootOf(*model)->remove(0, 1);
    model->commitTransaction();
    RecordingObserver observer;
    model->addObserver(&observer);

    model->reset(makeNode());
    const Log expected{"reset", "reset finished"};
    BOOST_CHECK_EQUAL_COLLECTIONS(observer.log.begin(), observer.log.end(), expected.begin(),
                                  expected.end());
    model->removeObserver(&observer);
}

BOOST_AUTO_TEST_CASE(test_destruction_reports_only_the_reset) {
    RecordingObserver observer;
    {
        auto model = makeModel(10, makeNode(1));
        model->beginTransaction();
        rootOf(*model)->remove(0, 1);
        model->commitTransaction();
        model->addObserver(&observer);
        // A transaction in progress is discarded without notifications of its nodes.
        model->beginTransaction();
        rootOf(*model)->append(makeNode());
        observer.log.clear();
    }
    const Log expected{"reset"};
    BOOST_CHECK_EQUAL_COLLECTIONS(observer.log.begin(), observer.log.end(), expected.begin(),
                                  expected.end());
}

BOOST_AUTO_TEST_CASE(test_an_empty_transaction_reports_nothing) {
    auto model = makeModel(10, makeNode());
    RecordingObserver observer;
    model->addObserver(&observer);
    model->beginTransaction();
    model->commitTransaction();
    BOOST_CHECK(observer.log.empty());
    model->removeObserver(&observer);
}

BOOST_AUTO_TEST_CASE(test_a_removed_observer_is_not_notified) {
    auto model = makeModel(10, makeNode());
    RecordingObserver first;
    RecordingObserver second;
    model->addObserver(&first);
    model->addObserver(&second);
    model->removeObserver(&first);

    model->beginTransaction();
    rootOf(*model)->append(makeNode());
    model->commitTransaction();
    BOOST_CHECK(first.log.empty());
    BOOST_CHECK_EQUAL(second.log.size(), 3u);
    model->removeObserver(&second);
}

BOOST_AUTO_TEST_SUITE_END()
