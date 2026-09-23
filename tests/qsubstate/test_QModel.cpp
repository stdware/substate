#include <cstdint>
#include <functional>
#include <random>
#include <set>
#include <string>
#include <vector>

#include <QtTest/QTest>

#include <substate/MemoryStorageEngine.h>
#include <substate/Model.h>
#include <substate/ModelObserver.h>

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
            const int kind = uniform(0, 21);
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
            if (kind < 18 && transfer(all)) {
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

            /// Every node except the root.
            std::vector<Node *> children;

            size_t size() const {
                return vectors.size() + structs.size() + mappings.size();
            }
        };

        static bool isAncestorOrSelf(const Node *node, const Node *target) {
            for (auto current = target; current; current = current->parent()) {
                if (current == node) {
                    return true;
                }
            }
            return false;
        }

        /// Transfers a random node to a random valid target: a VectorNode, an empty slot of a
        /// StructNode, or a missing key of a MappingNode. Returns false without an action if no
        /// valid target exists.
        bool transfer(const Nodes &all) {
            if (all.children.empty()) {
                return false;
            }
            auto node = pick(all.children);
            const auto valid = [&](const Node *target) {
                return target != node->parent() && !isAncestorOrSelf(node, target);
            };

            std::vector<std::function<void()>> targets;
            for (auto target : all.vectors) {
                if (valid(target)) {
                    targets.push_back([this, target, node] {
                        QVERIFY(target->transferIn(uniform(0, target->size()), node));
                    });
                }
            }
            for (auto target : all.structs) {
                for (int i = 0; i < target->size(); ++i) {
                    if (valid(target) && target->at(i).isEmpty()) {
                        targets.push_back(
                            [target, node, i] { QVERIFY(target->transferIn(i, node)); });
                    }
                }
            }
            for (auto target : all.mappings) {
                const QString key = randomKey();
                if (valid(target) && !target->contains(key)) {
                    targets.push_back(
                        [target, node, key] { QVERIFY(target->transferIn(key, node)); });
                }
            }
            if (targets.empty()) {
                return false;
            }
            pick(targets)();
            return true;
        }

        static void collect(Node *node, Nodes &out) {
            if (node->parent()) {
                out.children.push_back(node);
            }
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

namespace {

    /// An observer that compares the state of the model around each notification with the
    /// accessors of the action for the reported operation, and keeps the identifiers of the nodes
    /// that have been in the tree until their destruction is reported.
    class NotificationChecker : public ModelObserver {
    public:
        explicit NotificationChecker(const Model &model)
            : m_model(model), m_step(model.currentStep()) {
            collectIds(qdump(model.root()), m_live);
        }

        const std::set<std::uint64_t> &liveIds() const {
            return m_live;
        }

        int step() const {
            return m_step;
        }

        const std::string &error() const {
            return m_error;
        }

        void actionAboutToApply(const Action &action, Action::Operation operation) override {
            check(!m_pending, "nested actionAboutToApply()");
            m_pending = &action;
            checkState(action, operation, false);
        }

        void actionApplied(const Action &action, Action::Operation operation) override {
            check(m_pending == &action, "actionApplied() without actionAboutToApply()");
            m_pending = nullptr;
            checkState(action, operation, true);
            collectIds(qdump(m_model.root()), m_live);
        }

        void stepChanged(int step) override {
            m_step = step;
        }

        void nodeAboutToBeDestroyed(Node *node) override {
            check(!node->isAttached(), "destroyed node in the tree");
            check(m_live.erase(node->id()) == 1, "destroyed node unknown or reported twice");
        }

    private:
        void check(bool condition, const char *what) {
            if (!condition && m_error.empty()) {
                m_error = what;
            }
        }

        static bool holds(const Property &value, const QVariant &variant, const Node *child) {
            return value.variant() == variant && value.child() == child;
        }

        // Checks the state before the action is applied if applied is false, and after it
        // otherwise.
        void checkState(const Action &action, Action::Operation operation, bool applied) {
            switch (action.type()) {
                case Action::RootChange: {
                    const auto &a = static_cast<const RootChangeAction &>(action);
                    check(m_model.root() == (applied ? a.newRoot(operation) : a.oldRoot(operation)),
                          "root");
                    break;
                }
                case Action::VectorInsert:
                case Action::VectorRemove: {
                    const auto &a = static_cast<const VectorInsDelAction &>(action);
                    // The children are in the parent after an insertion and before a removal.
                    const bool inParent = a.isInsertion(operation) == applied;
                    for (size_t i = 0; i < a.children().size(); ++i) {
                        const auto child = a.children()[i];
                        check(inParent ? a.parent()->at(a.index() + int(i)) == child
                                       : !child->isAttached(),
                              "vector children");
                    }
                    break;
                }
                case Action::VectorMove: {
                    const auto &a = static_cast<const VectorMoveAction &>(action);
                    if (!applied) {
                        m_moved.clear();
                        for (int i = 0; i < a.count(); ++i) {
                            m_moved.push_back(a.parent()->at(a.index(operation) + i));
                        }
                    } else {
                        for (int i = 0; i < a.count(); ++i) {
                            check(a.parent()->at(a.destination(operation) + i) ==
                                      m_moved[size_t(i)],
                                  "moved children");
                        }
                    }
                    break;
                }
                case Action::Transfer: {
                    const auto &a = static_cast<const TransferAction &>(action);
                    for (auto node : a.nodes()) {
                        check(node->parent() ==
                                  (applied ? a.target(operation) : a.source(operation)),
                              "transfer");
                    }
                    break;
                }
                case Action::StructAssign: {
                    const auto &a = static_cast<const StructAssignAction &>(action);
                    const auto &value =
                        static_cast<const StructNodeBase *>(a.parent())->at(a.index());
                    check(applied ? holds(value, a.newVariant(operation), a.newChild(operation))
                                  : holds(value, a.oldVariant(operation), a.oldChild(operation)),
                          "struct slot");
                    break;
                }
                case Action::MappingAssign: {
                    const auto &a = static_cast<const MappingAssignAction &>(action);
                    const auto &value = static_cast<const MappingNode *>(a.parent())->at(a.key());
                    check(applied ? holds(value, a.newVariant(operation), a.newChild(operation))
                                  : holds(value, a.oldVariant(operation), a.oldChild(operation)),
                          "mapping entry");
                    break;
                }
                default:
                    check(false, "unexpected action type");
                    break;
            }
        }

        const Model &m_model;
        std::set<std::uint64_t> m_live;
        const Action *m_pending = nullptr;
        std::vector<Node *> m_moved;
        int m_step;
        std::string m_error;
    };

}

class test_QModel : public QObject {
    Q_OBJECT

private:
    static void verify(const Model &model, const Reference &reference,
                       const NotificationChecker &checker, bool inTransaction) {
        const auto ids = reference.liveIds();
        // The notifications agree with the changes and report every destroyed node.
        QCOMPARE(checker.error(), std::string());
        QVERIFY(checker.liveIds() == ids);
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
        if (auto root = model.root()) {
            QVERIFY(!root->parent());
            QVERIFY(qparentsConsistent(root));
        }
        if (!inTransaction) {
            QCOMPARE(qdump(model.root()), reference.current());
            QCOMPARE(model.currentStep() - model.minimumStep(), reference.executed());
            QCOMPARE(model.maximumStep() - model.minimumStep(), reference.retained());
            QCOMPARE(checker.step(), model.currentStep());
        }
    }

private Q_SLOTS:
    // The executable check of theorems 1 to 4 in docs/Design.md for the node types of qsubstate,
    // as test_random_history_matches_the_reference of the substate tests does for the others,
    // together with the check of the notifications.
    void random_history_matches_the_reference() {
        const int before = LiveNodes::count();
        {
            constexpr int stepLimit = 8;
            RandomEditor editor(20260923);
            // Declared before the model, which notifies it when destroyed.
            std::unique_ptr<NotificationChecker> checker;
            auto model = std::make_unique<Model>(std::make_unique<MemoryStorageEngine>(stepLimit));
            model->reset(editor.subtree(3));
            Reference reference(stepLimit, qdump(model->root()));
            checker = std::make_unique<NotificationChecker>(*model);
            model->addObserver(checker.get());
            verify(*model, reference, *checker, false);
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
                        verify(*model, reference, *checker, true);
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
                verify(*model, reference, *checker, false);
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
