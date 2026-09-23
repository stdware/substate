#include <QtTest/QTest>

#include <substate/Model.h>

#include <qsubstate/StructNode.h>

#include "QTestTree.h"

using namespace ss;

class test_StructNode : public QObject {
    Q_OBJECT

private:
    static std::unique_ptr<Model> makeModel() {
        auto model = std::make_unique<Model>();
        model->reset(std::make_unique<CountingStruct>());
        return model;
    }

    static StructNodeBase *rootOf(const Model &model) {
        return static_cast<StructNodeBase *>(model.root());
    }

    static void assign(Model &model, int index, Property value) {
        model.beginTransaction();
        rootOf(model)->setAt(index, std::move(value));
        model.commitTransaction();
    }

private Q_SLOTS:
    void a_free_node_is_modified_without_a_model() {
        CountingStruct node;
        QCOMPARE(node.size(), 3);
        QVERIFY(node.at(0).isEmpty());

        node.setAt(0, QVariant(qint64(7)));
        QCOMPARE(node.variant(0), QVariant(qint64(7)));

        auto child = makeNode();
        auto raw = child.get();
        node.setAt(1, std::move(child));
        QCOMPARE(node.child(1), raw);
        QCOMPARE(raw->parent(), &node);

        const int live = LiveNodes::count();
        Property taken = node.take(1);
        QCOMPARE(taken.child(), raw);
        QVERIFY(!raw->parent());
        QVERIFY(node.at(1).isEmpty());
        QCOMPARE(LiveNodes::count(), live);

        node.setAt(2, std::move(taken));
        node.setAt(2, QVariant(1.5));
        QCOMPARE(LiveNodes::count(), live - 1);
    }

    void an_assigned_child_is_restored_by_undo() {
        auto model = makeModel();
        auto node = rootOf(*model);

        auto first = makeNode(1);
        auto firstRaw = first.get();
        assign(*model, 0, std::move(first));
        const auto firstId = firstRaw->id();
        QVERIFY(firstId != 0);
        QVERIFY(firstRaw->isAttached() && firstRaw->child(0)->isAttached());

        auto second = makeNode();
        auto secondRaw = second.get();
        assign(*model, 0, std::move(second));
        QCOMPARE(node->child(0), secondRaw);
        QVERIFY(!firstRaw->isAttached() && !firstRaw->child(0)->isAttached());
        QCOMPARE(model->nodeById(firstId), firstRaw);

        model->undo();
        QCOMPARE(node->child(0), firstRaw);
        QVERIFY(firstRaw->isAttached());
        QVERIFY(!secondRaw->isAttached());

        model->undo();
        QVERIFY(node->at(0).isEmpty());
        QVERIFY(!firstRaw->isAttached());

        model->redo();
        model->redo();
        QCOMPARE(node->child(0), secondRaw);
        QVERIFY(secondRaw->isAttached());
    }

    // The previous implementation stored the old and the new value in exchanged members, and
    // reported the inserted nodes as removed. Here the action owns exactly the value that is not
    // in the node, which the count of live nodes and the index verify.
    void the_action_owns_the_value_outside_the_node() {
        const int before = LiveNodes::count();
        {
            auto model = makeModel();
            assign(*model, 1, makeNode(2));
            assign(*model, 1, QVariant(QStringLiteral("text")));
            QCOMPARE(model->nodeCount(), size_t(LiveNodes::count() - before));

            model->undo();
            QCOMPARE(model->nodeCount(), size_t(LiveNodes::count() - before));
            QVERIFY(rootOf(*model)->child(1)->isAttached());

            // The truncated assignment of the variant destroys nothing, and the replaced child
            // remains in the node.
            assign(*model, 2, QVariant(1.5));
            QCOMPARE(model->nodeCount(), size_t(LiveNodes::count() - before));
            QCOMPARE(LiveNodes::count() - before, 4);
        }
        QCOMPARE(LiveNodes::count(), before);
    }

    void a_variant_replaced_by_a_child_is_restored_by_undo() {
        auto model = makeModel();
        auto node = rootOf(*model);
        assign(*model, 2, QVariant(qint64(3)));
        assign(*model, 2, makeNode());
        QVERIFY(node->at(2).isChild());

        model->undo();
        QCOMPARE(node->variant(2), QVariant(qint64(3)));
        model->redo();
        QVERIFY(node->at(2).isChild());
    }

    void an_unchanged_value_creates_no_step() {
        auto model = makeModel();
        assign(*model, 0, QVariant(qint64(3)));
        assign(*model, 0, QVariant(qint64(3)));
        assign(*model, 1, Property());
        QCOMPARE(model->maximumStep(), 1);
    }

    void a_clone_copies_the_slots() {
        auto model = makeModel();
        assign(*model, 0, QVariant(1.5));
        assign(*model, 1, makeNode(1));

        auto copy = rootOf(*model)->clone();
        auto copied = dynamic_cast<CountingStruct *>(copy.get());
        QVERIFY(copied);
        QVERIFY(copied->isFree());
        QCOMPARE(copied->variant(0), QVariant(1.5));
        QVERIFY(copied->child(1) != rootOf(*model)->child(1));
        QCOMPARE(copied->child(1)->id(), std::uint64_t(0));
        QCOMPARE(copied->child(1)->parent(), copied);
        QVERIFY(copied->at(2).isEmpty());
    }
};

QTEST_APPLESS_MAIN(test_StructNode)

#include "test_StructNode.moc"
