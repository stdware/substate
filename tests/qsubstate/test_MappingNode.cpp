#include <QtTest/QTest>

#include <substate/Model.h>

#include <qsubstate/MappingNode.h>

#include "QTestTree.h"

using namespace ss;

class test_MappingNode : public QObject {
    Q_OBJECT

private:
    static std::unique_ptr<Model> makeModel() {
        auto model = std::make_unique<Model>();
        model->reset(std::make_unique<CountingMapping>());
        return model;
    }

    static MappingNode *rootOf(const Model &model) {
        return static_cast<MappingNode *>(model.root());
    }

    static bool assign(Model &model, const QString &key, Property value) {
        model.beginTransaction();
        const bool changed = rootOf(model)->setProperty(key, std::move(value));
        model.commitTransaction();
        return changed;
    }

private Q_SLOTS:
    void a_free_node_is_modified_without_a_model() {
        CountingMapping node;
        QVERIFY(node.setProperty(QStringLiteral("b"), QVariant(qint64(2))));
        QVERIFY(node.setProperty(QStringLiteral("a"), makeNode()));
        QCOMPARE(node.keys(), (QStringList{QStringLiteral("a"), QStringLiteral("b")}));
        QVERIFY(node.child(QStringLiteral("a"))->parent() == &node);

        const int live = LiveNodes::count();
        Property taken = node.take(QStringLiteral("a"));
        QVERIFY(taken.isChild());
        QVERIFY(!taken.child()->parent());
        QVERIFY(!node.contains(QStringLiteral("a")));
        QVERIFY(node.take(QStringLiteral("a")).isEmpty());
        QCOMPARE(LiveNodes::count(), live);

        QVERIFY(node.setProperty(QStringLiteral("b"), Property()));
        QCOMPARE(node.size(), 0);
        QVERIFY(!node.setProperty(QStringLiteral("b"), Property()));
    }

    void a_created_entry_is_removed_by_undo() {
        auto model = makeModel();
        auto node = rootOf(*model);
        QVERIFY(assign(*model, QStringLiteral("key"), QVariant(1.5)));
        QCOMPARE(node->variant(QStringLiteral("key")), QVariant(1.5));

        model->undo();
        QVERIFY(!node->contains(QStringLiteral("key")));
        QCOMPARE(node->size(), 0);

        model->redo();
        QCOMPARE(node->variant(QStringLiteral("key")), QVariant(1.5));
    }

    void a_removed_child_is_restored_by_undo() {
        auto model = makeModel();
        auto node = rootOf(*model);
        auto child = makeNode(1);
        auto raw = child.get();
        QVERIFY(assign(*model, QStringLiteral("child"), std::move(child)));
        const auto id = raw->id();

        QVERIFY(assign(*model, QStringLiteral("child"), Property()));
        QVERIFY(!node->contains(QStringLiteral("child")));
        QVERIFY(!raw->isAttached() && !raw->child(0)->isAttached());
        QVERIFY(model->nodeById(id) == raw);

        model->undo();
        QVERIFY(node->child(QStringLiteral("child")) == raw);
        QVERIFY(raw->isAttached());
        QCOMPARE(raw->id(), id);
    }

    void a_replaced_value_is_restored_by_undo() {
        auto model = makeModel();
        auto node = rootOf(*model);
        assign(*model, QStringLiteral("value"), QVariant(QStringLiteral("old")));
        assign(*model, QStringLiteral("value"), QVariant(QStringLiteral("new")));

        model->undo();
        QCOMPARE(node->variant(QStringLiteral("value")), QVariant(QStringLiteral("old")));
        model->redo();
        QCOMPARE(node->variant(QStringLiteral("value")), QVariant(QStringLiteral("new")));
    }

    void an_unchanged_value_creates_no_step() {
        auto model = makeModel();
        QVERIFY(assign(*model, QStringLiteral("value"), QVariant(qint64(1))));
        QVERIFY(!assign(*model, QStringLiteral("value"), QVariant(qint64(1))));
        QVERIFY(!assign(*model, QStringLiteral("missing"), Property()));
        QCOMPARE(model->maximumStep(), 1);
    }

    void a_clone_copies_the_entries() {
        auto model = makeModel();
        assign(*model, QStringLiteral("value"), QVariant(qint64(4)));
        assign(*model, QStringLiteral("child"), makeNode());

        auto copy = rootOf(*model)->clone();
        auto copied = dynamic_cast<CountingMapping *>(copy.get());
        QVERIFY(copied);
        QVERIFY(copied->isFree());
        QCOMPARE(copied->variant(QStringLiteral("value")), QVariant(qint64(4)));
        QVERIFY(copied->child(QStringLiteral("child")) !=
                rootOf(*model)->child(QStringLiteral("child")));
        QVERIFY(copied->child(QStringLiteral("child"))->parent() == copied);
    }
};

QTEST_APPLESS_MAIN(test_MappingNode)

#include "test_MappingNode.moc"
