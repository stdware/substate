#include <vector>

#include <substate/ArrayNode.h>
#include <substate/Model.h>

#include <boost/test/unit_test.hpp>

using namespace ss;

BOOST_AUTO_TEST_SUITE(test_ArrayNode)

namespace {

    using Curve = ArrayNode<double>;

    std::unique_ptr<Model> makeModel(const std::vector<double> &values) {
        auto node = std::make_unique<Curve>();
        node->append(values);
        auto model = std::make_unique<Model>();
        model->reset(std::move(node));
        return model;
    }

    Curve *rootOf(const Model &model) {
        return static_cast<Curve *>(model.root());
    }

}

BOOST_AUTO_TEST_CASE(test_elements_are_addressed_by_index) {
    Curve curve;
    curve.append(std::vector<double>{1.5, 2.5, 3.5});
    BOOST_CHECK_EQUAL(curve.size(), 3);
    BOOST_CHECK_EQUAL(static_cast<const BytesNode &>(curve).size(), 3 * int(sizeof(double)));
    BOOST_CHECK_EQUAL(curve.at(1), 2.5);

    curve.insert(1, std::vector<double>{-1.0});
    curve.remove(3, 1);
    BOOST_CHECK(curve.values() == (std::vector<double>{1.5, -1.0, 2.5}));

    curve.replace(2, std::vector<double>{7.0, 8.0});
    BOOST_CHECK(curve.values() == (std::vector<double>{1.5, -1.0, 7.0, 8.0}));

    curve.truncate(1);
    BOOST_CHECK(curve.values() == std::vector<double>{1.5});
}

BOOST_AUTO_TEST_CASE(test_changes_of_elements_are_reverted_by_undo) {
    auto model = makeModel({0.0, 1.0, 2.0, 3.0});
    auto curve = rootOf(*model);
    const auto original = curve->values();

    model->beginTransaction();
    curve->replace(3, std::vector<double>{30.0, 40.0});
    curve->remove(0, 1);
    curve->insert(0, std::vector<double>{-5.0});
    model->commitTransaction();
    BOOST_CHECK(curve->values() == (std::vector<double>{-5.0, 1.0, 2.0, 30.0, 40.0}));

    model->undo();
    BOOST_CHECK(curve->values() == original);
    model->redo();
    BOOST_CHECK(curve->values() == (std::vector<double>{-5.0, 1.0, 2.0, 30.0, 40.0}));
}

BOOST_AUTO_TEST_CASE(test_a_clone_keeps_the_element_type) {
    auto model = makeModel({4.0, 5.0});
    auto copy = rootOf(*model)->clone();
    auto copied = dynamic_cast<Curve *>(copy.get());
    BOOST_REQUIRE(copied);
    BOOST_CHECK(copied->isFree());
    BOOST_CHECK(copied->values() == (std::vector<double>{4.0, 5.0}));
}

BOOST_AUTO_TEST_SUITE_END()
