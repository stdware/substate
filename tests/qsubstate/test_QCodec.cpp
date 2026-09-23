#include <limits>
#include <memory>
#include <sstream>
#include <string>

#include <QtCore/QByteArray>
#include <QtCore/QPoint>
#include <QtCore/QString>
#include <QtCore/QVariant>
#include <QtTest/QTest>

#include <substate/MemoryStorageEngine.h>
#include <substate/Model.h>

#include <qsubstate/QCodec.h>

#include "CodecTestTools.h"
#include "QRandomEditor.h"
#include "QTestTree.h"

using namespace ss;

namespace {

    /// A value type without stream operators, which has no encoding.
    struct Opaque {
        int value = 0;
    };

    /// A QCodec for the counting node types of the tests.
    class TestQCodec : public QCodec {
    public:
        TestQCodec() {
            registerNodeType(Node::User, [] { return std::make_unique<CountingNode>(); });
            registerNodeType(Node::User + 3, [] { return std::make_unique<CountingStruct>(); });
            registerNodeType(Node::User + 4, [] { return std::make_unique<CountingMapping>(); });
        }
    };

    std::string encodeVariant(const QVariant &variant) {
        std::ostringstream stream(std::ios::binary);
        OBinaryStream out(stream);
        Encoder encoder(out);
        QCodec::writeVariant(encoder, variant);
        return encoder.fail() ? std::string() : stream.str();
    }

    QVariant decodeVariant(const std::string &bytes) {
        const QCodec codec;
        std::istringstream stream(bytes, std::ios::binary);
        IBinaryStream in(stream);
        Decoder decoder(codec, in, nullptr);
        auto variant = QCodec::readVariant(decoder);
        return stream.peek() == std::char_traits<char>::eof() ? variant : QVariant();
    }

    std::unique_ptr<Model> makeModel(int stepLimit) {
        return std::make_unique<Model>(std::make_unique<MemoryStorageEngine>(stepLimit));
    }

}

Q_DECLARE_METATYPE(Opaque)

class test_QCodec : public QObject {
    Q_OBJECT

private Q_SLOTS:
    void a_variant_round_trips_data() {
        QTest::addColumn<QVariant>("value");
        QTest::newRow("bool") << QVariant(true);
        QTest::newRow("int") << QVariant(-5);
        QTest::newRow("uint") << QVariant(std::numeric_limits<uint>::max());
        QTest::newRow("qint64") << QVariant(std::numeric_limits<qint64>::min());
        QTest::newRow("quint64") << QVariant(std::numeric_limits<quint64>::max());
        QTest::newRow("short") << QVariant::fromValue(short(-3));
        QTest::newRow("double") << QVariant(-1e300);
        // An emoji, an unpaired surrogate and a null character.
        QTest::newRow("string") << QVariant(QStringLiteral("a\U0001F600b") + QChar(0xD800) +
                                            QChar(0));
        QTest::newRow("empty string") << QVariant(QString());
        QTest::newRow("bytes") << QVariant(QByteArray("\0\x01\xff", 3));
        QTest::newRow("stream") << QVariant(QPoint(3, -4));
    }

    void a_variant_round_trips() {
        QFETCH(QVariant, value);
        const auto bytes = encodeVariant(value);
        QVERIFY(!bytes.empty());
        const auto decoded = decodeVariant(bytes);
        QCOMPARE(decoded.metaType(), value.metaType());
        QCOMPARE(decoded, value);
    }

    void a_value_without_stream_operators_has_no_encoding() {
        QVERIFY(encodeVariant(QVariant::fromValue(Opaque{1})).empty());
    }

    void an_unknown_encoding_is_rejected() {
        // An unknown kind, and a type name that QMetaType does not know.
        QVERIFY(!decodeVariant(std::string(1, '\x7f')).isValid());
        auto bytes = encodeVariant(QVariant(QPoint(1, 2)));
        const auto at = bytes.find("QPoint");
        QVERIFY(at != std::string::npos);
        bytes.replace(at, 6, "QPoinx");
        QVERIFY(!decodeVariant(bytes).isValid());
    }

    void a_tree_round_trips() {
        const TestQCodec codec;
        QRandomEditor editor(20260924);
        auto source = makeModel(10);
        source->reset(editor.subtree(4));
        auto target = makeModel(10);
        auto root = decodeNode(codec, encodeNode(source->root()), target.get());
        QVERIFY(root);
        target->restore(std::move(root));
        QCOMPARE(qdump(target->root()), qdump(source->root()));
        QCOMPARE(target->nodeCount(), source->nodeCount());
    }

    // The persistence check of test_Codec for the node types of qsubstate.
    void a_random_history_replays_from_its_encoding() {
        const TestQCodec codec;
        const int before = LiveNodes::count();
        {
            constexpr int stepLimit = 8;
            QRandomEditor editor(20260924);
            ActionRecorder recorder;
            auto source = makeModel(stepLimit);
            auto target = makeModel(stepLimit);
            source->reset(editor.subtree(3));
            target->restore(decodeNode(codec, encodeNode(source->root()), target.get()));
            QCOMPARE(qdump(target->root()), qdump(source->root()));
            source->addObserver(&recorder);

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
                        source->commitTransaction();
                        QVERIFY(!recorder.failed);
                        QVERIFY(replay(*target, codec, recorder.actions));
                    }
                } else if (choice < 8 ? source->canUndo() : !source->canRedo()) {
                    source->undo();
                    target->undo();
                } else {
                    source->redo();
                    target->redo();
                }
                QCOMPARE(qdump(target->root()), qdump(source->root()));
                QCOMPARE(target->nodeCount(), source->nodeCount());
                if (QTest::currentTestFailed()) {
                    break;
                }
            }
            source->removeObserver(&recorder);
        }
        QCOMPARE(LiveNodes::count(), before);
    }
};

QTEST_APPLESS_MAIN(test_QCodec)

#include "test_QCodec.moc"
