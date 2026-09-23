#include <substate/Model.h>
#include <substate/VectorNode.h>

#include <boost/test/unit_test.hpp>

#include "TestTree.h"

using namespace ss;

BOOST_AUTO_TEST_SUITE(test_VectorNode)

namespace {

    /// A model whose root has \a children leaves.
    std::unique_ptr<Model> makeModel(int children) {
        auto model = std::make_unique<Model>();
        model->reset(makeNode(children));
        return model;
    }

    CountingNode *rootOf(const Model &model) {
        return static_cast<CountingNode *>(model.root());
    }

    void move(Model &model, int index, int count, int destination) {
        model.beginTransaction();
        rootOf(model)->move(index, count, destination);
        model.commitTransaction();
    }

}

BOOST_AUTO_TEST_CASE(test_a_free_node_is_modified_without_a_model) {
    auto node = makeNode(3);
    auto first = node->child(0);
    auto last = node->child(2);
    BOOST_CHECK(first->isFree());
    BOOST_CHECK_EQUAL(first->parent(), node.get());
    BOOST_CHECK_EQUAL(first->id(), 0u);
    BOOST_CHECK(!first->isAttached());

    node->move(2, 1, 0);
    BOOST_CHECK_EQUAL(node->child(0), last);
    BOOST_CHECK_EQUAL(node->child(1), first);

    auto taken = node->take(1, 1);
    BOOST_REQUIRE_EQUAL(taken.size(), 1u);
    BOOST_CHECK_EQUAL(taken.front().get(), first);
    BOOST_CHECK(!first->parent());
    BOOST_CHECK_EQUAL(node->size(), 2);

    const int before = CountingNode::live();
    node->remove(0, 2);
    BOOST_CHECK_EQUAL(node->size(), 0);
    BOOST_CHECK_EQUAL(CountingNode::live(), before - 2);
}

BOOST_AUTO_TEST_CASE(test_an_undone_insertion_keeps_the_nodes_and_their_identity) {
    auto model = makeModel(0);
    auto root = rootOf(*model);

    auto inserted = makeNode(1);
    auto node = inserted.get();
    auto child = node->child(0);
    model->beginTransaction();
    root->append(std::move(inserted));
    model->commitTransaction();

    const auto id = node->id();
    const auto childId = child->id();
    BOOST_CHECK(id != 0 && childId != 0 && id != childId);
    BOOST_CHECK(node->isAttached() && child->isAttached());
    BOOST_CHECK_EQUAL(node->parent(), root);

    model->undo();
    BOOST_CHECK_EQUAL(root->size(), 0);
    BOOST_CHECK(!node->isAttached() && !child->isAttached());
    BOOST_CHECK(!node->parent());
    BOOST_CHECK_EQUAL(model->nodeById(id), node);
    BOOST_CHECK_EQUAL(model->nodeById(childId), child);

    model->redo();
    BOOST_REQUIRE_EQUAL(root->size(), 1);
    BOOST_CHECK_EQUAL(root->child(0), node);
    BOOST_CHECK_EQUAL(node->id(), id);
    BOOST_CHECK_EQUAL(node->child(0), child);
    BOOST_CHECK(node->isAttached() && child->isAttached());
}

BOOST_AUTO_TEST_CASE(test_an_undone_removal_restores_the_same_nodes) {
    auto model = makeModel(3);
    auto root = rootOf(*model);
    auto second = root->child(1);
    auto third = root->child(2);
    const auto secondId = second->id();

    model->beginTransaction();
    root->remove(1, 2);
    model->commitTransaction();
    BOOST_CHECK_EQUAL(root->size(), 1);
    BOOST_CHECK(!second->isAttached() && !third->isAttached());
    BOOST_CHECK_EQUAL(model->nodeById(secondId), second);

    model->undo();
    BOOST_REQUIRE_EQUAL(root->size(), 3);
    BOOST_CHECK_EQUAL(root->child(1), second);
    BOOST_CHECK_EQUAL(root->child(2), third);
    BOOST_CHECK_EQUAL(second->id(), secondId);
    BOOST_CHECK(second->isAttached());

    model->redo();
    BOOST_CHECK_EQUAL(root->size(), 1);
}

BOOST_AUTO_TEST_CASE(test_a_removed_subtree_is_detached_as_a_whole) {
    auto model = makeModel(0);
    auto root = rootOf(*model);
    model->beginTransaction();
    root->append(makeNode(2));
    model->commitTransaction();
    auto node = root->child(0);
    auto grandchild = node->child(1);

    model->beginTransaction();
    root->remove(0, 1);
    model->commitTransaction();
    BOOST_CHECK(!grandchild->isAttached());
    BOOST_CHECK_EQUAL(grandchild->parent(), node);

    model->undo();
    BOOST_CHECK(grandchild->isAttached());
}

BOOST_AUTO_TEST_CASE(test_a_move_is_reverted_by_undo) {
    auto model = makeModel(5);
    auto root = rootOf(*model);
    const std::string original = dump(root);

    // Forward: children 0 and 1 end at indices 2 and 3.
    move(*model, 0, 2, 2);
    const std::string forward = dump(root);
    BOOST_CHECK(forward != original);
    model->undo();
    BOOST_CHECK_EQUAL(dump(root), original);
    model->redo();
    BOOST_CHECK_EQUAL(dump(root), forward);

    // Backward: children 3 and 4 end at indices 0 and 1.
    auto fourth = root->child(3);
    move(*model, 3, 2, 0);
    BOOST_CHECK_EQUAL(root->child(0), fourth);
    model->undo();
    BOOST_CHECK_EQUAL(dump(root), forward);
}

BOOST_AUTO_TEST_CASE(test_a_clone_is_a_free_copy_without_identifiers) {
    auto model = makeModel(2);
    auto root = rootOf(*model);

    auto copy = root->clone();
    auto copied = static_cast<CountingNode *>(copy.get());
    BOOST_CHECK(copied->isFree());
    BOOST_CHECK(!copied->parent());
    BOOST_CHECK_EQUAL(copied->id(), 0u);
    BOOST_REQUIRE_EQUAL(copied->size(), 2);
    BOOST_CHECK(copied->child(0) != root->child(0));
    BOOST_CHECK_EQUAL(copied->child(0)->id(), 0u);
    BOOST_CHECK_EQUAL(copied->child(0)->parent(), copied);

    // The copy enters the model as new nodes.
    model->beginTransaction();
    root->append(std::move(copy));
    model->commitTransaction();
    BOOST_CHECK(copied->id() != 0);
    BOOST_CHECK(copied->child(0)->id() != root->child(0)->id());
}

BOOST_AUTO_TEST_SUITE_END()
