#include <cstdint>
#include <set>
#include <string>
#include <vector>

#include <QtTest/QTest>

#include <substate/MemoryStorageEngine.h>
#include <substate/Model.h>
#include <substate/ModelObserver.h>

#include "HistoryReference.h"
#include "QRandomEditor.h"
#include "QTestTree.h"

using namespace ss;

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
            QRandomEditor editor(20260923);
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
