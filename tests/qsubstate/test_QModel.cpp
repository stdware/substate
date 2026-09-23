#include <random>
#include <vector>

#include <QtTest/QTest>

#include <substate/MemoryStorageEngine.h>
#include <substate/Model.h>

#include "HistoryReference.h"
#include "QTestTree.h"

using namespace ss;

namespace {

    /// Random modifications of trees of VectorNode, StructNode and MappingNode objects.
    class RandomEditor {
    public:
        explicit RandomEditor(unsigned seed) : m_random(seed) {
        }

        int uniform(int low, int high) {
            return std::uniform_int_distribution<int>(low, high)(m_random);
        }

        std::unique_ptr<Node> subtree(int depth) {
            const int kind = uniform(0, 2);
            if (kind == 0) {
                auto node = std::make_unique<CountingStruct>();
                for (int i = 0; i < node->size(); ++i) {
                    node->setAt(i, randomValue(depth));
                }
                return node;
            }
            if (kind == 1) {
                auto node = std::make_unique<CountingMapping>();
                const int entries = uniform(0, 2);
                for (int i = 0; i < entries; ++i) {
                    node->setProperty(randomKey(), randomValue(depth));
                }
                return node;
            }
            auto node = std::make_unique<CountingNode>();
            const int children = depth > 0 ? uniform(0, 2) : 0;
            for (int i = 0; i < children; ++i) {
                node->append(subtree(depth - 1));
            }
            return node;
        }

        /// Performs one random modification within the current transaction. Every call creates
        /// exactly one action, so that a transaction is never empty.
        void modify(Model &model) {
            auto root = model.root();
            const int kind = uniform(0, 19);
            if (!root || kind == 0) {
                model.setRoot(subtree(2));
                return;
            }

            Nodes all;
            collect(root, all);

            std::vector<VectorNode *> removable;
            std::vector<VectorNode *> movable;
            for (auto node : all.vectors) {
                if (node->size() > 0) {
                    removable.push_back(node);
                }
                if (node->size() > 1) {
                    movable.push_back(node);
                }
            }

            if (kind < 6 && !all.vectors.empty() && all.size() < 40) {
                auto parent = pick(all.vectors);
                parent->insert(uniform(0, parent->size()), subtree(uniform(0, 2)));
                return;
            }
            if (kind < 9 && !removable.empty()) {
                auto parent = pick(removable);
                const int index = uniform(0, parent->size() - 1);
                parent->remove(index, uniform(1, parent->size() - index));
                return;
            }
            if (kind < 10 && !movable.empty()) {
                auto parent = pick(movable);
                const int size = parent->size();
                const int index = uniform(0, size - 2);
                int destination = index;
                while (destination == index) {
                    destination = uniform(0, size - 1);
                }
                parent->move(index, 1, destination);
                return;
            }
            if (kind < 15 && !all.structs.empty()) {
                auto node = pick(all.structs);
                const int index = uniform(0, node->size() - 1);
                node->setAt(index, changedValue(node->at(index)));
                return;
            }
            if (!all.mappings.empty()) {
                auto node = pick(all.mappings);
                const QString key = randomKey();
                QVERIFY(node->setProperty(key, changedValue(node->at(key))));
                return;
            }
            if (!all.structs.empty()) {
                auto node = pick(all.structs);
                node->setAt(0, changedValue(node->at(0)));
                return;
            }
            model.setRoot(subtree(2));
        }

    private:
        struct Nodes {
            std::vector<VectorNode *> vectors;
            std::vector<StructNodeBase *> structs;
            std::vector<MappingNode *> mappings;

            size_t size() const {
                return vectors.size() + structs.size() + mappings.size();
            }
        };

        static void collect(Node *node, Nodes &out) {
            if (auto vector = dynamic_cast<VectorNode *>(node)) {
                out.vectors.push_back(vector);
                for (int i = 0; i < vector->size(); ++i) {
                    collect(vector->at(i), out);
                }
            } else if (auto structNode = dynamic_cast<StructNodeBase *>(node)) {
                out.structs.push_back(structNode);
                for (int i = 0; i < structNode->size(); ++i) {
                    if (auto child = structNode->child(i)) {
                        collect(child, out);
                    }
                }
            } else if (auto mapping = dynamic_cast<MappingNode *>(node)) {
                out.mappings.push_back(mapping);
                for (const auto &key : mapping->keys()) {
                    if (auto child = mapping->child(key)) {
                        collect(child, out);
                    }
                }
            }
        }

        template <class T>
        T pick(const std::vector<T> &nodes) {
            return nodes[size_t(uniform(0, int(nodes.size()) - 1))];
        }

        QString randomKey() {
            return QString(QChar(u'a' + uniform(0, 3)));
        }

        /// A scalar value that differs from every previous one.
        QVariant uniqueVariant() {
            return QVariant(qint64(++m_serial));
        }

        Property randomValue(int depth) {
            const int kind = uniform(0, 2);
            if (kind == 1) {
                return uniqueVariant();
            }
            if (kind == 2 && depth > 0) {
                return subtree(depth - 1);
            }
            return {};
        }

        /// A value that differs from \a current, so that the assignment creates an action.
        Property changedValue(const Property &current) {
            const int kind = uniform(current.isEmpty() ? 1 : 0, 2);
            if (kind == 1) {
                return uniqueVariant();
            }
            if (kind == 2) {
                return subtree(uniform(0, 1));
            }
            return {};
        }

        std::mt19937 m_random;
        qint64 m_serial = 0;
    };

}

class test_QModel : public QObject {
    Q_OBJECT

private:
    static void verify(const Model &model, const Reference &reference, bool inTransaction) {
        const auto ids = reference.liveIds();
        std::set<std::uint64_t> attached;
        collectIds(reference.latest(), attached);
        QCOMPARE(model.nodeCount(), ids.size());
        QCOMPARE(LiveNodes::count(), int(ids.size()));
        for (auto id : ids) {
            auto node = model.nodeById(id);
            QVERIFY(node);
            QCOMPARE(node->id(), id);
            // A node is attached exactly if it is in the tree.
            QCOMPARE(node->isAttached(), attached.count(id) == 1);
        }
        if (!inTransaction) {
            QCOMPARE(qdump(model.root()), reference.current());
            QCOMPARE(model.currentStep() - model.minimumStep(), reference.executed());
            QCOMPARE(model.maximumStep() - model.minimumStep(), reference.retained());
        }
    }

private Q_SLOTS:
    // The executable check of theorems 1 to 4 in docs/Design.md for the node types of qsubstate,
    // as test_random_history_matches_the_reference of the substate tests does for the others.
    void random_history_matches_the_reference() {
        const int before = LiveNodes::count();
        {
            constexpr int stepLimit = 8;
            RandomEditor editor(20260923);
            auto model = std::make_unique<Model>(std::make_unique<MemoryStorageEngine>(stepLimit));
            model->reset(editor.subtree(3));
            Reference reference(stepLimit, qdump(model->root()));
            verify(*model, reference, false);
            if (QTest::currentTestFailed()) {
                return;
            }

            for (int round = 0; round < 3000; ++round) {
                const int choice = editor.uniform(0, 9);
                if (choice < 5 || (!model->canUndo() && !model->canRedo())) {
                    model->beginTransaction();
                    const int count = editor.uniform(1, 3);
                    for (int i = 0; i < count; ++i) {
                        editor.modify(*model);
                        reference.record(qdump(model->root()));
                        verify(*model, reference, true);
                        if (QTest::currentTestFailed()) {
                            return;
                        }
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
                if (QTest::currentTestFailed()) {
                    return;
                }
            }
        }
        QCOMPARE(LiveNodes::count(), before);
    }
};

QTEST_APPLESS_MAIN(test_QModel)

#include "test_QModel.moc"
