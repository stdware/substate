#include <string>

#include <substate/BytesNode.h>
#include <substate/Model.h>

#include <boost/test/unit_test.hpp>

#include "TestTree.h"

using namespace ss;

BOOST_AUTO_TEST_SUITE(test_BytesNode)

namespace {

    ArrayView<char> view(const std::string &text) {
        return ArrayView<char>(text.data(), text.size());
    }

    std::string textOf(const BytesNode &node) {
        return std::string(node.data().begin(), node.data().end());
    }

    // A model whose root is a CountingBytes with text.
    std::unique_ptr<Model> makeModel(const std::string &text) {
        auto node = std::make_unique<CountingBytes>();
        node->append(view(text));
        auto model = std::make_unique<Model>();
        model->reset(std::move(node));
        return model;
    }

    BytesNode *rootOf(const Model &model) {
        return static_cast<BytesNode *>(model.root());
    }

}

BOOST_AUTO_TEST_CASE(test_a_free_node_is_modified_without_a_model) {
    CountingBytes node;
    node.append(view("abcdef"));
    node.insert(3, view("XY"));
    BOOST_CHECK_EQUAL(textOf(node), "abcXYdef");
    node.remove(0, 2);
    BOOST_CHECK_EQUAL(textOf(node), "cXYdef");
    node.replace(4, view("1234"));
    BOOST_CHECK_EQUAL(textOf(node), "cXYd1234");
    node.truncate(3);
    BOOST_CHECK_EQUAL(textOf(node), "cXY");
    node.clear();
    BOOST_CHECK_EQUAL(node.size(), 0);
}

BOOST_AUTO_TEST_CASE(test_insertion_and_removal_are_reverted_by_undo) {
    auto model = makeModel("abcdef");
    auto node = rootOf(*model);

    model->beginTransaction();
    node->insert(2, view("XYZ"));
    model->commitTransaction();
    BOOST_CHECK_EQUAL(textOf(*node), "abXYZcdef");

    model->beginTransaction();
    node->remove(0, 4);
    model->commitTransaction();
    BOOST_CHECK_EQUAL(textOf(*node), "Zcdef");

    model->undo();
    BOOST_CHECK_EQUAL(textOf(*node), "abXYZcdef");
    model->undo();
    BOOST_CHECK_EQUAL(textOf(*node), "abcdef");
    model->redo();
    model->redo();
    BOOST_CHECK_EQUAL(textOf(*node), "Zcdef");
}

BOOST_AUTO_TEST_CASE(test_a_replacement_within_the_array_is_reverted_by_undo) {
    auto model = makeModel("abcdef");
    auto node = rootOf(*model);

    model->beginTransaction();
    node->replace(1, view("XY"));
    model->commitTransaction();
    BOOST_CHECK_EQUAL(textOf(*node), "aXYdef");

    model->undo();
    BOOST_CHECK_EQUAL(textOf(*node), "abcdef");
    model->redo();
    BOOST_CHECK_EQUAL(textOf(*node), "aXYdef");
}

// The previous implementation inserted the extension at an index computed from the length of the
// new bytes rather than of the array, and replaced the bytes after it.
BOOST_AUTO_TEST_CASE(test_a_replacement_beyond_the_end_extends_the_array) {
    auto model = makeModel("abcdef");
    auto node = rootOf(*model);

    model->beginTransaction();
    node->replace(4, view("WXYZ"));
    model->commitTransaction();
    BOOST_CHECK_EQUAL(textOf(*node), "abcdWXYZ");

    model->undo();
    BOOST_CHECK_EQUAL(textOf(*node), "abcdef");
    model->redo();
    BOOST_CHECK_EQUAL(textOf(*node), "abcdWXYZ");

    model->beginTransaction();
    node->replace(node->size(), view("!"));
    model->commitTransaction();
    BOOST_CHECK_EQUAL(textOf(*node), "abcdWXYZ!");
    model->undo();
    BOOST_CHECK_EQUAL(textOf(*node), "abcdWXYZ");
}

BOOST_AUTO_TEST_CASE(test_an_empty_insertion_creates_no_step) {
    auto model = makeModel("abc");
    model->beginTransaction();
    rootOf(*model)->insert(1, ArrayView<char>());
    model->commitTransaction();
    BOOST_CHECK_EQUAL(model->maximumStep(), 0);
}

BOOST_AUTO_TEST_CASE(test_a_clone_copies_the_bytes) {
    auto model = makeModel("abc");
    auto copy = rootOf(*model)->clone();
    auto copied = dynamic_cast<CountingBytes *>(copy.get());
    BOOST_REQUIRE(copied);
    BOOST_CHECK(copied->isFree());
    BOOST_CHECK_EQUAL(copied->id(), 0u);
    BOOST_CHECK_EQUAL(textOf(*copied), "abc");
}

BOOST_AUTO_TEST_SUITE_END()
