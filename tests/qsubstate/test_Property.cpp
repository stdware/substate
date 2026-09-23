#include <QtTest/QTest>

#include <qsubstate/Property.h>

#include "QTestTree.h"

using namespace ss;

class test_Property : public QObject {
    Q_OBJECT

private Q_SLOTS:
    void an_invalid_variant_is_stored_as_empty() {
        QVERIFY(Property().isEmpty());
        QVERIFY(Property(QVariant()).isEmpty());
        QVERIFY(Property(std::unique_ptr<Node>()).isEmpty());
        QVERIFY(!Property(QVariant()).variant().isValid());
        QVERIFY(!Property().child());
    }

    void a_variant_and_a_child_are_distinguished() {
        const Property variant(QVariant(qint64(5)));
        QVERIFY(variant.isVariant());
        QCOMPARE(variant.variant(), QVariant(qint64(5)));
        QVERIFY(!variant.child());

        auto node = makeNode();
        auto raw = node.get();
        const Property child(std::move(node));
        QVERIFY(child.isChild());
        QCOMPARE(child.child(), raw);
        QVERIFY(!child.variant().isValid());
    }

    void moving_transfers_the_child() {
        const int live = LiveNodes::count();
        Property source(makeNode());
        auto raw = source.child();

        Property target(std::move(source));
        QCOMPARE(target.child(), raw);

        Property other;
        other = std::move(target);
        QCOMPARE(other.child(), raw);
        QCOMPARE(LiveNodes::count(), live + 1);

        other = Property(QVariant(QStringLiteral("text")));
        QCOMPARE(LiveNodes::count(), live);
    }

    void a_clone_copies_the_child_as_a_free_node() {
        auto node = makeNode(2);
        auto raw = node.get();
        const Property original(std::move(node));

        const Property copy = original.clone();
        QVERIFY(copy.isChild());
        QVERIFY(copy.child() != raw);
        QVERIFY(copy.child()->isFree());
        QVERIFY(!copy.child()->parent());
        QCOMPARE(static_cast<VectorNode *>(copy.child())->size(), 2);

        const Property variant(QVariant(1.5));
        QCOMPARE(variant.clone().variant(), QVariant(1.5));
        QVERIFY(Property().clone().isEmpty());
    }

    // Children are compared by identity, because two nodes with the same content are still
    // different nodes in the tree.
    void equality_compares_values_and_child_identity() {
        QVERIFY(Property() == Property());
        QVERIFY(Property(QVariant(1.5)) == Property(QVariant(1.5)));
        QVERIFY(Property(QVariant(1.5)) != Property(QVariant(2.5)));
        QVERIFY(Property(QVariant(1.5)) != Property());

        const Property first(makeNode());
        const Property second(makeNode());
        QVERIFY(first == first);
        QVERIFY(first != second);
        QVERIFY(first != first.clone());
    }
};

QTEST_APPLESS_MAIN(test_Property)

#include "test_Property.moc"
