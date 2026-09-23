#include <vector>

#include <substate/Model.h>
#include <substate/SheetNode.h>

#include <boost/test/unit_test.hpp>

#include "TestTree.h"

using namespace ss;

BOOST_AUTO_TEST_SUITE(test_SheetNode)

namespace {

    // A model whose root is an empty CountingSheet.
    std::unique_ptr<Model> makeModel() {
        auto model = std::make_unique<Model>();
        model->reset(std::make_unique<CountingSheet>());
        return model;
    }

    SheetNode *rootOf(const Model &model) {
        return static_cast<SheetNode *>(model.root());
    }

    int insert(Model &model, std::unique_ptr<Node> node) {
        model.beginTransaction();
        const int key = rootOf(model)->insert(std::move(node));
        model.commitTransaction();
        return key;
    }

}

BOOST_AUTO_TEST_CASE(test_a_free_node_assigns_increasing_keys) {
    CountingSheet sheet;
    auto first = makeNode();
    auto firstNode = first.get();
    BOOST_CHECK_EQUAL(sheet.insert(std::move(first)), 1);
    BOOST_CHECK_EQUAL(sheet.insert(makeNode()), 2);
    BOOST_CHECK_EQUAL(sheet.insert(makeNode()), 3);
    BOOST_CHECK_EQUAL(firstNode->parent(), &sheet);
    BOOST_CHECK(firstNode->isFree());

    const int before = CountingNode::live();
    BOOST_CHECK(sheet.remove(2));
    BOOST_CHECK(!sheet.remove(2));
    BOOST_CHECK_EQUAL(CountingNode::live(), before - 1);

    auto taken = sheet.take(1);
    BOOST_CHECK_EQUAL(taken.get(), firstNode);
    BOOST_CHECK(!firstNode->parent());
    BOOST_CHECK(!sheet.take(1));
    BOOST_CHECK(sheet.keys() == std::vector<int>{3});

    // A removed key is not assigned again.
    BOOST_CHECK_EQUAL(sheet.insert(makeNode()), 4);
}

BOOST_AUTO_TEST_CASE(test_an_undone_insertion_keeps_the_key_and_the_node) {
    auto model = makeModel();
    auto sheet = rootOf(*model);

    auto inserted = makeNode(1);
    auto node = inserted.get();
    const int key = insert(*model, std::move(inserted));
    const auto id = node->id();
    BOOST_CHECK(id != 0);
    BOOST_CHECK_EQUAL(sheet->at(key), node);
    BOOST_CHECK(node->isAttached() && node->child(0)->isAttached());

    model->undo();
    BOOST_CHECK(!sheet->at(key));
    BOOST_CHECK_EQUAL(sheet->size(), 0);
    BOOST_CHECK(!node->isAttached() && !node->child(0)->isAttached());
    BOOST_CHECK_EQUAL(model->nodeById(id), node);

    model->redo();
    BOOST_CHECK_EQUAL(sheet->at(key), node);
    BOOST_CHECK_EQUAL(node->id(), id);
    BOOST_CHECK(node->isAttached());
}

// The previous implementation looked up the key with the identifier of the sheet itself, and
// removed the child without executing the action, unlike the undo path.
BOOST_AUTO_TEST_CASE(test_an_undone_removal_restores_the_node_under_its_key) {
    auto model = makeModel();
    auto sheet = rootOf(*model);
    insert(*model, makeNode());
    auto second = makeNode();
    auto node = second.get();
    const int key = insert(*model, std::move(second));
    BOOST_REQUIRE(key != int(sheet->id()));

    model->beginTransaction();
    BOOST_CHECK(sheet->remove(key));
    model->commitTransaction();
    BOOST_CHECK(!sheet->at(key));
    BOOST_CHECK_EQUAL(sheet->size(), 1);
    BOOST_CHECK(!node->isAttached());

    model->undo();
    BOOST_CHECK_EQUAL(sheet->at(key), node);
    BOOST_CHECK(node->isAttached());

    model->redo();
    BOOST_CHECK(!sheet->at(key));
}

BOOST_AUTO_TEST_CASE(test_removing_a_missing_key_creates_no_step) {
    auto model = makeModel();
    model->beginTransaction();
    BOOST_CHECK(!rootOf(*model)->remove(7));
    model->commitTransaction();
    BOOST_CHECK_EQUAL(model->maximumStep(), 0);
}

// Keys are not restored by undo or abort, so that a key once returned never refers to another
// node, even if the caller kept it past the undo.
BOOST_AUTO_TEST_CASE(test_keys_are_not_reused_after_undo_or_abort) {
    auto model = makeModel();
    auto sheet = rootOf(*model);
    BOOST_CHECK_EQUAL(insert(*model, makeNode()), 1);
    model->undo();
    BOOST_CHECK_EQUAL(insert(*model, makeNode()), 2);

    model->beginTransaction();
    BOOST_CHECK_EQUAL(sheet->insert(makeNode()), 3);
    model->abortTransaction();
    BOOST_CHECK_EQUAL(insert(*model, makeNode()), 4);
    BOOST_CHECK(sheet->keys() == (std::vector<int>{2, 4}));
}

BOOST_AUTO_TEST_CASE(test_a_clone_keeps_the_keys_without_identifiers) {
    auto model = makeModel();
    auto sheet = rootOf(*model);
    insert(*model, makeNode());
    insert(*model, makeNode());
    model->beginTransaction();
    sheet->remove(1);
    model->commitTransaction();

    auto copy = sheet->clone();
    auto copied = static_cast<SheetNode *>(copy.get());
    BOOST_CHECK(dynamic_cast<CountingSheet *>(copied));
    BOOST_CHECK(copied->isFree());
    BOOST_CHECK(copied->keys() == std::vector<int>{2});
    BOOST_CHECK_EQUAL(copied->at(2)->id(), 0u);
    BOOST_CHECK_EQUAL(copied->at(2)->parent(), copied);
    BOOST_CHECK_EQUAL(copied->insert(makeNode()), 3);
}

// The sheet under a vector root, and a leaf of the root that is transferred into the sheet and
// out of it again.
BOOST_AUTO_TEST_CASE(test_a_transfer_assigns_a_key_that_redo_keeps) {
    auto model = std::make_unique<Model>();
    model->reset(makeNode(1));
    auto root = static_cast<CountingNode *>(model->root());
    auto leaf = root->child(0);
    model->beginTransaction();
    root->append(std::make_unique<CountingSheet>());
    model->commitTransaction();
    auto sheet = static_cast<SheetNode *>(root->at(1));
    const auto id = leaf->id();

    model->beginTransaction();
    const int key = sheet->transferIn(leaf);
    model->commitTransaction();
    BOOST_CHECK_EQUAL(key, 1);
    BOOST_CHECK_EQUAL(sheet->at(key), leaf);
    BOOST_CHECK_EQUAL(leaf->parent(), sheet);
    BOOST_CHECK_EQUAL(root->size(), 1);

    model->undo();
    BOOST_CHECK(!sheet->at(key));
    BOOST_CHECK_EQUAL(root->child(0), leaf);
    model->redo();
    BOOST_CHECK_EQUAL(sheet->at(key), leaf);
    BOOST_CHECK_EQUAL(leaf->id(), id);

    // Out of the sheet. The key is not assigned again afterwards.
    model->beginTransaction();
    BOOST_CHECK(root->transferIn(0, leaf));
    model->commitTransaction();
    BOOST_CHECK_EQUAL(sheet->size(), 0);
    model->undo();
    BOOST_CHECK_EQUAL(sheet->at(key), leaf);
    model->redo();
    model->beginTransaction();
    BOOST_CHECK_EQUAL(sheet->insert(makeNode()), 2);
    model->commitTransaction();
}

BOOST_AUTO_TEST_SUITE_END()
