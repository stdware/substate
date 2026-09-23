#include <algorithm>
#include <cstdint>
#include <random>
#include <string>
#include <vector>

#include <substate/MemoryStorageEngine.h>
#include <substate/Model.h>

#include <boost/test/unit_test.hpp>

#include "HistoryReference.h"
#include "TestTree.h"

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

    /// Commits one transaction that appends a leaf to \a target, for filling the history.
    void appendLeaf(Model &model, CountingNode *target) {
        model.beginTransaction();
        target->append(makeNode());
        model.commitTransaction();
    }

    class RandomEditor {
    public:
        explicit RandomEditor(unsigned seed) : m_random(seed) {
        }

        int uniform(int low, int high) {
            return std::uniform_int_distribution<int>(low, high)(m_random);
        }

        /// A random free subtree of VectorNode, SheetNode and BytesNode objects.
        std::unique_ptr<Node> subtree(int depth) {
            const int children = depth > 0 ? uniform(0, 2) : 0;
            if (uniform(0, 4) == 0) {
                auto bytes = std::make_unique<CountingBytes>();
                bytes->append(randomBytes());
                return bytes;
            }
            if (uniform(0, 3) == 0) {
                auto sheet = std::make_unique<CountingSheet>();
                for (int i = 0; i < children; ++i) {
                    sheet->insert(subtree(depth - 1));
                }
                return sheet;
            }
            auto node = std::make_unique<CountingNode>();
            for (int i = 0; i < children; ++i) {
                node->append(subtree(depth - 1));
            }
            return node;
        }

        /// Performs one random modification within the current transaction. Every call creates
        /// exactly one action, so that a transaction is never empty.
        void modify(Model &model) {
            auto root = model.root();
            if (!root) {
                model.setRoot(subtree(2));
                return;
            }

            Containers all;
            collect(root, all);

            const int kind = uniform(0, 21);
            if (kind == 0) {
                // A null root occasionally, which leaves an empty tree.
                model.setRoot(uniform(0, 3) == 0 ? nullptr : subtree(2));
                return;
            }

            // The root is a BytesNode, and the tree has no node that can have children.
            if (all.size() == 0) {
                editBytes(pick(all.bytes));
                return;
            }

            if (kind < 9 && all.size() < 40) {
                insert(all);
                return;
            }

            Containers removable;
            std::vector<VectorNode *> movable;
            for (auto node : all.vectors) {
                if (node->size() > 0) {
                    removable.vectors.push_back(node);
                }
                if (node->size() > 1) {
                    movable.push_back(node);
                }
            }
            for (auto node : all.sheets) {
                if (node->size() > 0) {
                    removable.sheets.push_back(node);
                }
            }

            if (kind < 16 && removable.size() > 0) {
                remove(removable);
                return;
            }

            if (kind < 18 && !all.bytes.empty()) {
                editBytes(pick(all.bytes));
                return;
            }

            if (kind < 20 && transfer(all)) {
                return;
            }

            if (!movable.empty()) {
                auto parent = pick(movable);
                const int size = parent->size();
                const int index = uniform(0, size - 1);
                // At most size - 1 children, so that a destination other than index exists.
                const int count = uniform(1, std::min(size - index, size - 1));
                int destination = index;
                while (destination == index) {
                    destination = uniform(0, size - count);
                }
                parent->move(index, count, destination);
                return;
            }

            insert(all);
        }

    private:
        struct Containers {
            std::vector<VectorNode *> vectors;
            std::vector<SheetNode *> sheets;
            std::vector<BytesNode *> bytes;

            /// Every node except the root.
            std::vector<Node *> children;

            /// The number of nodes that can have children.
            size_t size() const {
                return vectors.size() + sheets.size();
            }
        };

        std::vector<char> randomBytes() {
            std::vector<char> result(size_t(uniform(1, 4)));
            for (auto &byte : result) {
                byte = char(uniform(0, 255));
            }
            return result;
        }

        /// Inserts, removes or replaces bytes. A replacement may extend the array, which creates
        /// a replacement and an insertion.
        void editBytes(BytesNode *node) {
            const int size = node->size();
            const int kind = uniform(0, 2);
            if (kind == 0 || size == 0) {
                node->insert(uniform(0, size), randomBytes());
            } else if (kind == 1) {
                const int index = uniform(0, size - 1);
                node->remove(index, uniform(1, size - index));
            } else {
                node->replace(uniform(0, size), randomBytes());
            }
        }

        static void collect(Node *node, Containers &out) {
            if (node->parent()) {
                out.children.push_back(node);
            }
            if (auto vector = dynamic_cast<VectorNode *>(node)) {
                out.vectors.push_back(vector);
                for (int i = 0; i < vector->size(); ++i) {
                    collect(vector->at(i), out);
                }
            } else if (auto sheet = dynamic_cast<SheetNode *>(node)) {
                out.sheets.push_back(sheet);
                for (int key : sheet->keys()) {
                    collect(sheet->at(key), out);
                }
            } else if (auto bytes = dynamic_cast<BytesNode *>(node)) {
                out.bytes.push_back(bytes);
            }
        }

        template <class T>
        T pick(const std::vector<T> &nodes) {
            return nodes[size_t(uniform(0, int(nodes.size()) - 1))];
        }

        void insert(const Containers &all) {
            const int choice = uniform(0, int(all.size()) - 1);
            if (choice >= int(all.vectors.size())) {
                all.sheets[size_t(choice) - all.vectors.size()]->insert(subtree(uniform(0, 2)));
                return;
            }
            auto parent = all.vectors[size_t(choice)];
            std::vector<std::unique_ptr<Node>> inserted;
            const int count = uniform(1, 2);
            for (int i = 0; i < count; ++i) {
                inserted.push_back(subtree(uniform(0, 2)));
            }
            parent->insert(uniform(0, parent->size()), std::move(inserted));
        }

        static bool isAncestorOrSelf(const Node *node, const Node *target) {
            for (auto current = target; current; current = current->parent()) {
                if (current == node) {
                    return true;
                }
            }
            return false;
        }

        /// Transfers a random node, or a range of consecutive children of a VectorNode, to a
        /// random valid target. Returns false without an action if no valid target exists.
        bool transfer(const Containers &all) {
            if (all.children.empty()) {
                return false;
            }
            std::vector<Node *> nodes{pick(all.children)};
            const auto parent = nodes.front()->parent();
            if (auto vector = dynamic_cast<VectorNode *>(parent)) {
                int index = 0;
                while (vector->at(index) != nodes.front()) {
                    ++index;
                }
                const int count = uniform(1, std::min(3, vector->size() - index));
                for (int i = 1; i < count; ++i) {
                    nodes.push_back(vector->at(index + i));
                }
            }

            const auto valid = [&](const Node *target) {
                if (target == parent) {
                    return false;
                }
                for (auto node : nodes) {
                    if (isAncestorOrSelf(node, target)) {
                        return false;
                    }
                }
                return true;
            };
            std::vector<VectorNode *> vectors;
            std::vector<SheetNode *> sheets;
            for (auto node : all.vectors) {
                if (valid(node)) {
                    vectors.push_back(node);
                }
            }
            // A SheetNode receives one node per transfer.
            if (nodes.size() == 1) {
                for (auto node : all.sheets) {
                    if (valid(node)) {
                        sheets.push_back(node);
                    }
                }
            }

            if (vectors.empty() && sheets.empty()) {
                return false;
            }
            const int choice = uniform(0, int(vectors.size() + sheets.size()) - 1);
            if (choice < int(vectors.size())) {
                auto target = vectors[size_t(choice)];
                BOOST_REQUIRE(target->transferIn(uniform(0, target->size()), nodes));
            } else {
                BOOST_REQUIRE(sheets[size_t(choice) - vectors.size()]->transferIn(nodes.front()) >
                              0);
            }
            return true;
        }

        void remove(const Containers &removable) {
            const int choice = uniform(0, int(removable.size()) - 1);
            if (choice >= int(removable.vectors.size())) {
                auto sheet = removable.sheets[size_t(choice) - removable.vectors.size()];
                BOOST_REQUIRE(sheet->remove(pick(sheet->keys())));
                return;
            }
            auto parent = removable.vectors[size_t(choice)];
            const int index = uniform(0, parent->size() - 1);
            parent->remove(index, uniform(1, parent->size() - index));
        }

        std::mt19937 m_random;
    };

    void verify(const Model &model, const Reference &reference, bool inTransaction) {
        const auto ids = reference.liveIds();
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
// configuration of the current step.
BOOST_AUTO_TEST_CASE(test_random_history_matches_the_reference) {
    const int before = CountingNode::live();
    {
        constexpr int stepLimit = 8;
        RandomEditor editor(20260923);
        auto model = makeModel(stepLimit, editor.subtree(3));
        Reference reference(stepLimit, dump(model->root()));
        verify(*model, reference, false);

        for (int round = 0; round < 3000; ++round) {
            const int choice = editor.uniform(0, 9);
            if (choice < 5 || (!model->canUndo() && !model->canRedo())) {
                model->beginTransaction();
                const int count = editor.uniform(1, 3);
                for (int i = 0; i < count; ++i) {
                    editor.modify(*model);
                    reference.record(dump(model->root()));
                    verify(*model, reference, true);
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
            verify(*model, reference, false);
        }
    }
    BOOST_CHECK_EQUAL(CountingNode::live(), before);
}

BOOST_AUTO_TEST_SUITE_END()
