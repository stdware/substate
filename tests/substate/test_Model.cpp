#include <cstdint>
#include <memory>
#include <string>

#include <substate/MemoryStorageEngine.h>
#include <substate/Model.h>

#include <boost/test/unit_test.hpp>

#include "HistoryReference.h"
#include "RandomEditor.h"
#include "TestTree.h"
#include "TreeMirror.h"

using namespace ss;

BOOST_AUTO_TEST_SUITE(test_Model)

namespace {

    std::unique_ptr<Model> makeModel(int stepLimit, std::unique_ptr<Node> root) {
        auto model = std::make_unique<Model>(std::make_unique<MemoryStorageEngine>(stepLimit));
        model->reset(std::move(root));
        return model;
    }

    CountingNode *rootOf(const Model &model) {
        return static_cast<CountingNode *>(model.root());
    }

    // Commits one transaction that appends a leaf to target, for filling the history.
    void appendLeaf(Model &model, CountingNode *target) {
        model.beginTransaction();
        target->append(makeNode());
        model.commitTransaction();
    }

    void verify(const Model &model, const Reference &reference, const TreeMirror &mirror,
                bool inTransaction) {
        const auto ids = reference.liveIds();
        // The notifications reproduce the tree and report every destroyed node.
        BOOST_REQUIRE_EQUAL(mirror.error(), "");
        BOOST_REQUIRE_EQUAL(mirror.dump(), dump(model.root()));
        BOOST_REQUIRE(mirror.liveIds() == ids);
        std::set<std::uint64_t> attached;
        collectIds(reference.latest(), attached);
        BOOST_REQUIRE_EQUAL(model.nodeCount(), ids.size());
        BOOST_REQUIRE_EQUAL(CountingNode::live(), int(ids.size()));
        for (auto id : ids) {
            auto node = model.nodeById(id);
            BOOST_REQUIRE(node);
            BOOST_REQUIRE_EQUAL(node->id(), id);
            // A node is attached exactly if it is in the tree.
            BOOST_REQUIRE_EQUAL(node->isAttached(), attached.count(id) == 1);
        }
        if (auto root = model.root()) {
            BOOST_REQUIRE(!root->parent());
            BOOST_REQUIRE(parentsConsistent(root));
        }
        if (!inTransaction) {
            BOOST_REQUIRE_EQUAL(dump(model.root()), reference.current());
            BOOST_REQUIRE_EQUAL(model.currentStep() - model.minimumStep(), reference.executed());
            BOOST_REQUIRE_EQUAL(mirror.step(), model.currentStep());
            BOOST_REQUIRE_EQUAL(model.maximumStep() - model.minimumStep(), reference.retained());
        }
    }

}

BOOST_AUTO_TEST_CASE(test_reset_installs_the_initial_tree_without_a_step) {
    auto model = makeModel(10, makeNode(2));
    auto root = rootOf(*model);
    BOOST_CHECK(root->isAttached() && root->child(1)->isAttached());
    BOOST_CHECK(root->id() != 0);
    BOOST_CHECK_EQUAL(model->nodeCount(), 3u);
    BOOST_CHECK_EQUAL(model->nodeById(root->child(1)->id()), root->child(1));
    BOOST_CHECK(!model->canUndo());
    BOOST_CHECK_EQUAL(model->maximumStep(), 0);
}

BOOST_AUTO_TEST_CASE(test_a_replaced_root_is_restored_by_undo) {
    auto model = makeModel(10, makeNode(1));
    auto oldRoot = rootOf(*model);
    const auto oldId = oldRoot->id();

    auto replacement = makeNode();
    auto newRoot = replacement.get();
    model->beginTransaction();
    model->setRoot(std::move(replacement));
    model->commitTransaction();
    BOOST_CHECK_EQUAL(model->root(), newRoot);
    BOOST_CHECK(!oldRoot->isAttached() && !oldRoot->child(0)->isAttached());
    BOOST_CHECK_EQUAL(model->nodeById(oldId), oldRoot);

    model->undo();
    BOOST_CHECK_EQUAL(model->root(), oldRoot);
    BOOST_CHECK(oldRoot->isAttached());
    BOOST_CHECK(!newRoot->isAttached());

    model->redo();
    BOOST_CHECK_EQUAL(model->root(), newRoot);
}

BOOST_AUTO_TEST_CASE(test_an_aborted_transaction_restores_the_tree) {
    auto model = makeModel(10, makeNode(3));
    auto root = rootOf(*model);
    const std::string before = dump(root);
    const int live = CountingNode::live();

    model->beginTransaction();
    root->append(makeNode(2));
    root->remove(0, 1);
    root->move(0, 1, 2);
    model->abortTransaction();

    BOOST_CHECK_EQUAL(dump(root), before);
    BOOST_CHECK(!model->canUndo());
    // The inserted nodes are destroyed with the aborted actions.
    BOOST_CHECK_EQUAL(CountingNode::live(), live);
    BOOST_CHECK_EQUAL(model->nodeCount(), 4u);
}

BOOST_AUTO_TEST_CASE(test_an_empty_transaction_creates_no_step) {
    auto model = makeModel(10, makeNode());
    model->beginTransaction();
    model->commitTransaction({
        {"message", "nothing"}
    });
    BOOST_CHECK_EQUAL(model->maximumStep(), 0);
    BOOST_CHECK(!model->inTransaction());
}

BOOST_AUTO_TEST_CASE(test_the_message_is_kept_with_the_step) {
    auto model = makeModel(10, makeNode());
    model->beginTransaction();
    rootOf(*model)->append(makeNode());
    model->commitTransaction({
        {"message", "Insert a node"}
    });
    BOOST_CHECK_EQUAL(model->stepMessage(1).at("message"), "Insert a node");
}

BOOST_AUTO_TEST_CASE(test_an_undone_insertion_is_destroyed_when_truncated) {
    auto model = makeModel(10, makeNode());
    auto root = rootOf(*model);
    model->beginTransaction();
    root->append(makeNode(1));
    model->commitTransaction();
    const auto id = root->child(0)->id();
    const int live = CountingNode::live();

    model->undo();
    BOOST_CHECK(model->nodeById(id));

    appendLeaf(*model, root);
    BOOST_CHECK(!model->nodeById(id));
    BOOST_CHECK_EQUAL(CountingNode::live(), live - 2 + 1);
}

// The first defect of AceTreeModel in docs/References.md. There, evicting the step that inserted
// X freed X, although the later step that removed X was still in the history, and undoing that
// removal inserted the freed X into the tree. Here the removal owns X, and evicting the insertion
// destroys nothing.
BOOST_AUTO_TEST_CASE(test_a_removed_node_survives_eviction_of_its_insertion) {
    auto model = makeModel(100, makeNode(1));
    auto root = rootOf(*model);
    auto filler = root->child(0);

    model->beginTransaction();
    root->append(makeNode(1));
    model->commitTransaction();
    auto x = root->child(1);
    auto xChild = x->child(0);
    const auto xId = x->id();

    while (model->currentStep() < 149) {
        appendLeaf(*model, filler);
    }
    model->beginTransaction();
    root->remove(1, 1);
    model->commitTransaction();
    const int removal = model->currentStep();
    BOOST_REQUIRE_EQUAL(removal, 150);

    while (model->currentStep() < 205) {
        appendLeaf(*model, filler);
    }
    BOOST_REQUIRE_EQUAL(model->minimumStep(), 105);

    while (model->currentStep() > removal - 1) {
        model->undo();
    }
    BOOST_REQUIRE_EQUAL(root->size(), 2);
    BOOST_CHECK_EQUAL(root->child(1), x);
    BOOST_CHECK_EQUAL(x->id(), xId);
    BOOST_CHECK_EQUAL(x->child(0), xChild);
    BOOST_CHECK(x->isAttached() && xChild->isAttached());

    while (model->canUndo()) {
        model->undo();
    }
    while (model->canRedo()) {
        model->redo();
    }
    BOOST_CHECK_EQUAL(root->size(), 1);
    BOOST_CHECK_EQUAL(model->nodeCount(), size_t(CountingNode::live()));
}

// The second defect of AceTreeModel in docs/References.md. There, a node of the initial tree has
// no insertion step, so nothing freed it after its removal. Here the removal owns it, and the
// node is destroyed when the removal is evicted.
BOOST_AUTO_TEST_CASE(test_a_removed_initial_node_is_destroyed_with_its_removal) {
    auto model = makeModel(100, makeNode(2));
    auto root = rootOf(*model);
    auto filler = root->child(0);
    const auto yId = root->child(1)->id();
    const int live = CountingNode::live();

    model->beginTransaction();
    root->remove(1, 1);
    model->commitTransaction();
    BOOST_CHECK(model->nodeById(yId));

    while (model->minimumStep() < 1) {
        appendLeaf(*model, filler);
    }
    BOOST_CHECK(!model->nodeById(yId));
    // The removed node is destroyed, and the leaves appended to the filler exist.
    BOOST_CHECK_EQUAL(CountingNode::live(), live - 1 + (model->currentStep() - 1));
    BOOST_CHECK_EQUAL(model->nodeCount(), size_t(CountingNode::live()));
}

// The check of transfer in docs/Design.md: a node is transferred to another parent, the former
// parent is removed, and the steps before the removal are evicted. Undoing the removal restores
// the former parent, and the transferred node remains alive under its new parent.
BOOST_AUTO_TEST_CASE(test_a_transferred_node_survives_the_eviction_of_the_transfer) {
    auto tree = makeNode();
    auto former = makeNode(1);
    auto formerRaw = former.get();
    auto node = former->child(0);
    tree->append(std::move(former));
    tree->append(makeNode());
    tree->append(makeNode());
    auto model = makeModel(2, std::move(tree));
    auto root = rootOf(*model);
    auto target = root->child(1);
    auto filler = root->child(2);
    const auto nodeId = node->id();
    const auto formerId = formerRaw->id();

    model->beginTransaction();
    BOOST_REQUIRE(target->transferIn(0, node));
    model->commitTransaction();
    model->beginTransaction();
    root->remove(0, 1);
    model->commitTransaction();
    appendLeaf(*model, filler);
    BOOST_REQUIRE_EQUAL(model->minimumStep(), 1);

    model->undo();
    model->undo();
    BOOST_CHECK(!model->canUndo());
    BOOST_CHECK_EQUAL(root->child(0), formerRaw);
    BOOST_CHECK_EQUAL(formerRaw->id(), formerId);
    BOOST_CHECK(formerRaw->isAttached());
    BOOST_CHECK_EQUAL(formerRaw->size(), 0);
    BOOST_CHECK_EQUAL(target->child(0), node);
    BOOST_CHECK_EQUAL(node->id(), nodeId);
    BOOST_CHECK(node->isAttached());
    BOOST_CHECK_EQUAL(model->nodeCount(), size_t(CountingNode::live()));
}

BOOST_AUTO_TEST_CASE(test_every_node_is_destroyed_by_reset_and_destruction) {
    const int before = CountingNode::live();
    {
        auto model = makeModel(3, makeNode(2));
        auto root = rootOf(*model);
        for (int i = 0; i < 5; ++i) {
            model->beginTransaction();
            root->append(makeNode(1));
            root->remove(0, 1);
            model->commitTransaction();
        }
        model->undo();

        model->reset(makeNode(1));
        BOOST_CHECK_EQUAL(model->nodeCount(), 2u);
        BOOST_CHECK_EQUAL(CountingNode::live(), before + 2);
        BOOST_CHECK_EQUAL(model->maximumStep(), 0);

        model->beginTransaction();
        rootOf(*model)->append(makeNode());
        model->commitTransaction();
    }
    BOOST_CHECK_EQUAL(CountingNode::live(), before);
}

// Identifiers are not reused after a reset, so that an identifier kept by the caller never
// refers to a different node.
BOOST_AUTO_TEST_CASE(test_identifiers_are_not_reused_after_reset) {
    auto model = makeModel(10, makeNode());
    const auto first = model->root()->id();
    model->reset(makeNode());
    BOOST_CHECK(model->root()->id() > first);
    BOOST_CHECK(!model->nodeById(first));
}

// The executable check of theorems 1 to 4 in docs/Design.md: after every action, undo and redo,
// the live nodes equal the nodes of the retained configurations, and the tree equals the recorded
// configuration of the current step. A TreeMirror checks that the notifications reproduce every
// change and report every destroyed node.
BOOST_AUTO_TEST_CASE(test_random_history_matches_the_reference) {
    const int before = CountingNode::live();
    {
        constexpr int stepLimit = 8;
        RandomEditor editor(20260923);
        // Declared before the model, which notifies it when destroyed, also if a check throws.
        std::unique_ptr<TreeMirror> mirror;
        auto model = makeModel(stepLimit, editor.subtree(3));
        Reference reference(stepLimit, dump(model->root()));
        mirror = std::make_unique<TreeMirror>(*model);
        model->addObserver(mirror.get());
        verify(*model, reference, *mirror, false);

        for (int round = 0; round < 3000; ++round) {
            const int choice = editor.uniform(0, 9);
            if (choice < 5 || (!model->canUndo() && !model->canRedo())) {
                model->beginTransaction();
                const int count = editor.uniform(1, 3);
                for (int i = 0; i < count; ++i) {
                    editor.modify(*model);
                    reference.record(dump(model->root()));
                    verify(*model, reference, *mirror, true);
                }
                if (editor.uniform(0, 9) == 0) {
                    model->abortTransaction();
                    reference.abort();
                } else {
                    model->commitTransaction();
                    reference.commit();
                }
            } else if (choice < 8 ? model->canUndo() : !model->canRedo()) {
                model->undo();
                reference.undo();
            } else {
                model->redo();
                reference.redo();
            }
            verify(*model, reference, *mirror, false);
        }
    }
    BOOST_CHECK_EQUAL(CountingNode::live(), before);
}

BOOST_AUTO_TEST_SUITE_END()
