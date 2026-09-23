#include <memory>

#include <QtCore/QStringList>
#include <QtTest/QTest>

#include <substate/MemoryStorageEngine.h>
#include <substate/Model.h>

#include <qsubstate/ModelNotifier.h>

#include "QTestTree.h"

using namespace ss;

class test_ModelNotifier : public QObject {
    Q_OBJECT

private:
    // Connects every signal of notifier to a lambda that appends a line to log.
    static void record(ModelNotifier *notifier, QStringList *log) {
        const auto operationName = [](Action::Operation operation) {
            return operation == Action::Execute ? QStringLiteral("execute")
                   : operation == Action::Undo  ? QStringLiteral("undo")
                                                : QStringLiteral("redo");
        };
        QObject::connect(notifier, &ModelNotifier::actionAboutToApply,
                         [=](const Action *action, Action::Operation operation) {
                             *log << QStringLiteral("about %1 %2")
                                         .arg(action->type())
                                         .arg(operationName(operation));
                         });
        QObject::connect(notifier, &ModelNotifier::actionApplied,
                         [=](const Action *action, Action::Operation operation) {
                             *log << QStringLiteral("applied %1 %2")
                                         .arg(action->type())
                                         .arg(operationName(operation));
                         });
        QObject::connect(notifier, &ModelNotifier::stepChanged,
                         [=](int step) { *log << QStringLiteral("step %1").arg(step); });
        QObject::connect(notifier, &ModelNotifier::nodeAboutToBeDestroyed,
                         [=](Node *node) { *log << QStringLiteral("destroy %1").arg(node->id()); });
        QObject::connect(notifier, &ModelNotifier::aboutToReset,
                         [=] { *log << QStringLiteral("reset"); });
        QObject::connect(notifier, &ModelNotifier::resetFinished,
                         [=] { *log << QStringLiteral("reset finished"); });
    }

private Q_SLOTS:
    void every_notification_is_emitted() {
        Model model(std::make_unique<MemoryStorageEngine>(10));
        QStringList log;
        auto notifier = std::make_unique<ModelNotifier>(&model);
        QCOMPARE(notifier->model(), &model);
        record(notifier.get(), &log);

        model.reset(std::make_unique<CountingStruct>());
        auto root = static_cast<CountingStruct *>(model.root());
        model.beginTransaction();
        root->setAt(0, QVariant(1));
        model.commitTransaction();
        model.undo();
        model.redo();
        model.beginTransaction();
        root->setAt(1, std::make_unique<CountingStruct>());
        const auto child = root->child(1)->id();
        model.abortTransaction();

        const QString assign = QString::number(Action::StructAssign);
        const QStringList expected{
            QStringLiteral("reset"),
            QStringLiteral("reset finished"),
            QStringLiteral("about %1 execute").arg(assign),
            QStringLiteral("applied %1 execute").arg(assign),
            QStringLiteral("step 1"),
            QStringLiteral("about %1 undo").arg(assign),
            QStringLiteral("applied %1 undo").arg(assign),
            QStringLiteral("step 0"),
            QStringLiteral("about %1 redo").arg(assign),
            QStringLiteral("applied %1 redo").arg(assign),
            QStringLiteral("step 1"),
            QStringLiteral("about %1 execute").arg(assign),
            QStringLiteral("applied %1 execute").arg(assign),
            QStringLiteral("about %1 undo").arg(assign),
            QStringLiteral("applied %1 undo").arg(assign),
            QStringLiteral("destroy %1").arg(child),
        };
        QCOMPARE(log, expected);
    }

    void a_destroyed_notifier_is_removed_from_the_model() {
        Model model(std::make_unique<MemoryStorageEngine>(10));
        model.reset(std::make_unique<CountingStruct>());
        QStringList log;
        {
            ModelNotifier notifier(&model);
            record(&notifier, &log);
        }
        model.beginTransaction();
        static_cast<CountingStruct *>(model.root())->setAt(0, QVariant(1));
        model.commitTransaction();
        QVERIFY(log.isEmpty());
    }
};

QTEST_APPLESS_MAIN(test_ModelNotifier)

#include "test_ModelNotifier.moc"
