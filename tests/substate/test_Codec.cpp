#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include <substate/Codec.h>
#include <substate/MemoryStorageEngine.h>
#include <substate/Model.h>
#include <substate/StorageEngine.h>

#include <boost/test/unit_test.hpp>

#include "CodecTestTools.h"
#include "RandomEditor.h"
#include "TestTree.h"

using namespace ss;

BOOST_AUTO_TEST_SUITE(test_Codec)

namespace {

    /// A Codec for the counting node types of the tests.
    class TestCodec : public Codec {
    public:
        TestCodec() {
            registerNodeType(Node::User, [] { return std::make_unique<CountingNode>(); });
            registerNodeType(Node::User + 1, [] { return std::make_unique<CountingSheet>(); });
            registerNodeType(Node::User + 2, [] { return std::make_unique<CountingBytes>(); });
        }
    };

    std::unique_ptr<Model> makeModel(int stepLimit) {
        return std::make_unique<Model>(std::make_unique<MemoryStorageEngine>(stepLimit));
    }

    /// Returns a tree with every counting node type, and a SheetNode whose key counter exceeds
    /// its keys, because a child was removed.
    std::unique_ptr<Node> makeTree() {
        auto root = makeNode(1);
        auto sheet = std::make_unique<CountingSheet>();
        sheet->insert(makeNode(2));
        const int removed = sheet->insert(makeNode());
        sheet->insert(std::make_unique<CountingBytes>());
        sheet->remove(removed);
        root->append(std::move(sheet));
        auto bytes = std::make_unique<CountingBytes>();
        bytes->append(std::vector<char>{'\0', '\x7f', '\xff'});
        root->append(std::move(bytes));
        return root;
    }

}

BOOST_AUTO_TEST_CASE(test_a_tree_round_trips_with_identifiers_and_keys) {
    const TestCodec codec;
    auto source = makeModel(10);
    source->reset(makeTree());
    const auto bytes = encodeNode(source->root());
    BOOST_REQUIRE(!bytes.empty());

    auto target = makeModel(10);
    auto root = decodeNode(codec, bytes, target.get());
    BOOST_REQUIRE(root);
    BOOST_CHECK(!root->isAttached());
    BOOST_CHECK_EQUAL(target->nodeCount(), source->nodeCount());
    target->restore(std::move(root));
    BOOST_CHECK_EQUAL(dump(target->root()), dump(source->root()));
    BOOST_CHECK(target->root()->isAttached());

    // The key counter and the identifier counter continue from the decoded values.
    for (auto model : {source.get(), target.get()}) {
        model->beginTransaction();
        auto sheet = static_cast<CountingSheet *>(static_cast<VectorNode *>(model->root())->at(1));
        sheet->insert(makeNode());
        model->commitTransaction();
    }
    BOOST_CHECK_EQUAL(dump(target->root()), dump(source->root()));
}

BOOST_AUTO_TEST_CASE(test_the_identifier_counter_is_restored_from_the_record) {
    const TestCodec codec;
    auto source = makeModel(10);
    source->reset(makeNode());
    auto target = makeModel(10);
    target->restore(decodeNode(codec, encodeNode(source->root()), target.get()), 100);

    target->beginTransaction();
    static_cast<VectorNode *>(target->root())->append(makeNode());
    target->commitTransaction();
    BOOST_CHECK_EQUAL(static_cast<VectorNode *>(target->root())->at(0)->id(), 101u);
}

BOOST_AUTO_TEST_CASE(test_a_tree_decodes_as_free_nodes_without_a_model) {
    const TestCodec codec;
    auto source = makeModel(10);
    source->reset(makeTree());
    auto node = decodeNode(codec, encodeNode(source->root()), nullptr);
    BOOST_REQUIRE(node);
    BOOST_CHECK(node->isFree());
    // A free copy has the structure of the source with identifiers 0.
    BOOST_CHECK_EQUAL(dump(node.get()), dump(source->root()->clone().get()));
}

BOOST_AUTO_TEST_CASE(test_invalid_input_is_rejected_without_remains) {
    const TestCodec codec;
    auto source = makeModel(10);
    source->reset(makeTree());
    const auto bytes = encodeNode(source->root());
    const int live = CountingNode::live();

    // Every truncation fails, and the nodes decoded partially are destroyed.
    auto target = makeModel(10);
    for (size_t size = 0; size < bytes.size(); ++size) {
        BOOST_CHECK(!decodeNode(codec, bytes.substr(0, size), target.get()));
        BOOST_REQUIRE_EQUAL(target->nodeCount(), 0u);
    }
    BOOST_CHECK_EQUAL(CountingNode::live(), live);

    // A node type without a factory.
    BOOST_CHECK(!decodeNode(Codec(), bytes, target.get()));

    // Identifiers that exist in the model already.
    auto decoded = decodeNode(codec, bytes, target.get());
    BOOST_REQUIRE(decoded);
    const auto count = target->nodeCount();
    BOOST_CHECK(!decodeNode(codec, bytes, target.get()));
    BOOST_CHECK_EQUAL(target->nodeCount(), count);
}

BOOST_AUTO_TEST_CASE(test_an_action_referring_to_a_missing_node_is_rejected) {
    const TestCodec codec;
    auto source = makeModel(10);
    source->reset(makeNode(1));
    ActionRecorder recorder;
    source->addObserver(&recorder);
    source->beginTransaction();
    static_cast<VectorNode *>(source->root())->remove(0, 1);
    source->commitTransaction();
    source->removeObserver(&recorder);
    BOOST_REQUIRE_EQUAL(recorder.actions.size(), 1u);

    auto target = makeModel(10);
    BOOST_CHECK(!decodeAction(codec, recorder.actions.front(), target.get()));
    BOOST_CHECK(!decodeAction(codec, recorder.actions.front(), nullptr));
}

namespace {

    /// Replays the addition of a child to an empty SheetNode, by insertion or by transfer, and
    /// returns the key of a later insertion into the restored SheetNode.
    int keyAfterReplay(bool transfer) {
        const TestCodec codec;
        auto tree = makeNode(1);
        tree->append(std::make_unique<CountingSheet>());
        auto source = makeModel(10);
        source->reset(std::move(tree));
        auto target = makeModel(10);
        target->restore(decodeNode(codec, encodeNode(source->root()), target.get()));

        ActionRecorder recorder;
        source->addObserver(&recorder);
        source->beginTransaction();
        auto root = static_cast<CountingNode *>(source->root());
        auto sheet = static_cast<CountingSheet *>(root->at(1));
        if (transfer) {
            BOOST_REQUIRE_EQUAL(sheet->transferIn(root->at(0)), 1);
        } else {
            BOOST_REQUIRE_EQUAL(sheet->insert(makeNode()), 1);
        }
        source->commitTransaction();
        source->removeObserver(&recorder);
        BOOST_REQUIRE(replay(*target, codec, recorder.actions));

        auto targetRoot = static_cast<VectorNode *>(target->root());
        target->beginTransaction();
        const int key = static_cast<CountingSheet *>(targetRoot->at(targetRoot->size() - 1))
                            ->insert(makeNode());
        target->commitTransaction();
        return key;
    }

}

// A replayed insertion or transfer carries its key. The key counter of the restored SheetNode
// covers it afterwards, so that a later insertion receives a new key.
BOOST_AUTO_TEST_CASE(test_keys_continue_after_replayed_actions) {
    BOOST_CHECK_EQUAL(keyAfterReplay(false), 2);
    BOOST_CHECK_EQUAL(keyAfterReplay(true), 2);
}

// An applied action takes the nodes it owns from the pool, and fails without them.
BOOST_AUTO_TEST_CASE(test_an_applied_removal_requires_its_nodes_in_the_pool) {
    const TestCodec codec;
    auto source = makeModel(10);
    source->reset(makeNode(1));
    const auto removedId = static_cast<VectorNode *>(source->root())->at(0)->id();
    ActionRecorder recorder;
    source->addObserver(&recorder);
    source->beginTransaction();
    static_cast<VectorNode *>(source->root())->remove(0, 1);
    source->commitTransaction();
    source->removeObserver(&recorder);
    const auto removed = encodeNode(source->nodeById(removedId));

    auto target = makeModel(10);
    target->restore(decodeNode(codec, encodeNode(source->root()), target.get()));
    NodePool pool;
    BOOST_CHECK(
        !decodeAction(codec, recorder.actions.front(), target.get(), &pool, Action::Applied));

    BOOST_REQUIRE(pool.add(decodeNode(codec, removed, target.get())));
    auto action =
        decodeAction(codec, recorder.actions.front(), target.get(), &pool, Action::Applied);
    BOOST_REQUIRE(action);
    BOOST_CHECK(pool.empty());
    std::vector<Node *> held;
    action->forEachHeldNode([&held](Node *node) { held.push_back(node); });
    BOOST_REQUIRE_EQUAL(held.size(), 1u);
    BOOST_CHECK_EQUAL(held.front()->id(), removedId);
}

// The executable check of the persistence of actions, in two forms.
//
// Every action of a random history is encoded at its first execution. A second model starts
// from the encoded initial tree and replays the decoded actions, and has the same tree, with the
// same identifiers and keys, after every step, also through undo and redo.
//
// Periodically, a third model is restored from a checkpoint of the tree and of the nodes that
// the applied actions own, and from the encoded retained transactions, decoded in their present
// state. It follows the source model through every retained step.
BOOST_AUTO_TEST_CASE(test_a_random_history_replays_from_its_encoding) {
    const TestCodec codec;
    const int before = CountingNode::live();
    {
        constexpr int stepLimit = 8;
        RandomEditor editor(20260924);
        ActionRecorder recorder;
        auto engine = new LoggingEngine(stepLimit);
        auto source = std::make_unique<Model>(std::unique_ptr<StorageEngine>(engine));
        auto target = makeModel(stepLimit);
        source->reset(editor.subtree(3));
        target->restore(decodeNode(codec, encodeNode(source->root()), target.get()));
        BOOST_REQUIRE_EQUAL(dump(target->root()), dump(source->root()));
        source->addObserver(&recorder);

        int restores = 0;
        for (int round = 0; round < 3000; ++round) {
            const int choice = editor.uniform(0, 9);
            if (choice < 5 || (!source->canUndo() && !source->canRedo())) {
                recorder.actions.clear();
                source->beginTransaction();
                const int count = editor.uniform(1, 3);
                for (int i = 0; i < count; ++i) {
                    editor.modify(*source);
                }
                if (editor.uniform(0, 9) == 0) {
                    source->abortTransaction();
                } else {
                    engine->pending = recorder.actions;
                    source->commitTransaction();
                    BOOST_REQUIRE(!recorder.failed);
                    BOOST_REQUIRE(replay(*target, codec, recorder.actions));
                }
            } else if (choice < 8 ? source->canUndo() : !source->canRedo()) {
                source->undo();
                target->undo();
            } else {
                source->redo();
                target->redo();
            }
            BOOST_REQUIRE_EQUAL(dump(target->root()), dump(source->root()));
            BOOST_REQUIRE_EQUAL(target->nodeCount(), source->nodeCount());
            BOOST_REQUIRE_EQUAL(target->currentStep(), source->currentStep());

            if (round % 25 == 0) {
                auto restored = restoreCopy(codec, *source, *engine);
                BOOST_REQUIRE(restored);
                ++restores;
                BOOST_REQUIRE(sweepHistory(*source, *restored, [&] {
                    return dump(restored->root()) == dump(source->root()) &&
                           restored->nodeCount() == source->nodeCount() &&
                           restored->currentStep() == source->currentStep();
                }));
            }
        }
        BOOST_CHECK_EQUAL(restores, 120);
        source->removeObserver(&recorder);
    }
    BOOST_CHECK_EQUAL(CountingNode::live(), before);
}

BOOST_AUTO_TEST_SUITE_END()
