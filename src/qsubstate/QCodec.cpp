#include "QCodec.h"

#include <cstdint>
#include <cstring>
#include <vector>

#include <QtCore/QByteArray>
#include <QtCore/QIODevice>
#include <QtCore/QMetaType>

#include "MappingNode.h"
#include "StructNode.h"

namespace ss {

    namespace {

        // The encodings of a QVariant.
        enum VariantKind : uint8_t {
            BoolKind = 1,
            IntegerKind,
            DoubleKind,
            StringKind,
            ByteArrayKind,
            StreamKind,
        };

        bool isSignedInteger(int typeId) {
            switch (typeId) {
                case QMetaType::Int:
                case QMetaType::LongLong:
                case QMetaType::Short:
                case QMetaType::Long:
                case QMetaType::Char:
                case QMetaType::SChar:
                    return true;
                default:
                    break;
            }
            return false;
        }

        bool isUnsignedInteger(int typeId) {
            switch (typeId) {
                case QMetaType::UInt:
                case QMetaType::ULongLong:
                case QMetaType::UShort:
                case QMetaType::ULong:
                case QMetaType::UChar:
                    return true;
                default:
                    break;
            }
            return false;
        }

        ArrayView<char> viewOf(const QByteArray &bytes) {
            return ArrayView<char>(bytes.constData(), size_t(bytes.size()));
        }

        QByteArray byteArrayOf(const std::vector<char> &bytes) {
            return QByteArray(bytes.data(), qsizetype(bytes.size()));
        }

    }

    QCodec::QCodec() {
        registerNodeType(Node::Mapping, [] { return std::make_unique<MappingNode>(); });
        registerActionType(Action::StructAssign, &StructAssignAction::read);
        registerActionType(Action::MappingAssign, &MappingAssignAction::read);
    }

    QCodec::~QCodec() = default;

    void QCodec::writeVariant(Encoder &encoder, const QVariant &variant) {
        auto &out = encoder.stream();
        const int typeId = variant.metaType().id();
        if (typeId == QMetaType::Bool) {
            out << uint8_t(BoolKind) << variant.toBool();
        } else if (isSignedInteger(typeId)) {
            out << uint8_t(IntegerKind) << int32_t(typeId) << int64_t(variant.toLongLong());
        } else if (isUnsignedInteger(typeId)) {
            out << uint8_t(IntegerKind) << int32_t(typeId) << uint64_t(variant.toULongLong());
        } else if (typeId == QMetaType::Double) {
            out << uint8_t(DoubleKind) << variant.toDouble();
        } else if (typeId == QMetaType::QString) {
            out << uint8_t(StringKind);
            writeString(encoder, variant.toString());
        } else if (typeId == QMetaType::QByteArray) {
            out << uint8_t(ByteArrayKind);
            encoder.writeBytes(viewOf(variant.toByteArray()));
        } else {
            const auto metaType = variant.metaType();
            QByteArray payload;
            QDataStream stream(&payload, QIODevice::WriteOnly);
            stream.setVersion(dataStreamVersion);
            if (!metaType.isValid() || !metaType.name() ||
                !metaType.save(stream, variant.constData()) || stream.status() != QDataStream::Ok) {
                encoder.setFailed();
                return;
            }
            out << uint8_t(StreamKind);
            encoder.writeBytes(viewOf(QByteArray(metaType.name())));
            encoder.writeBytes(viewOf(payload));
        }
    }

    QVariant QCodec::readVariant(Decoder &decoder) {
        auto &in = decoder.stream();
        uint8_t kind = 0;
        in >> kind;
        QVariant result;
        switch (kind) {
            case BoolKind: {
                bool value = false;
                in >> value;
                result = value;
                break;
            }
            case IntegerKind: {
                int32_t typeId = 0;
                in >> typeId;
                if (isSignedInteger(typeId)) {
                    int64_t value = 0;
                    in >> value;
                    result = QVariant::fromValue(qint64(value));
                } else if (isUnsignedInteger(typeId)) {
                    uint64_t value = 0;
                    in >> value;
                    result = QVariant::fromValue(quint64(value));
                } else {
                    decoder.setFailed();
                    break;
                }
                if (!result.convert(QMetaType(typeId))) {
                    decoder.setFailed();
                }
                break;
            }
            case DoubleKind: {
                double value = 0;
                in >> value;
                result = value;
                break;
            }
            case StringKind:
                result = readString(decoder);
                break;
            case ByteArrayKind:
                result = byteArrayOf(decoder.readBytes());
                break;
            case StreamKind: {
                const auto name = byteArrayOf(decoder.readBytes());
                const auto payload = byteArrayOf(decoder.readBytes());
                const auto metaType = QMetaType::fromName(name);
                if (decoder.fail() || !metaType.isValid()) {
                    decoder.setFailed();
                    break;
                }
                QDataStream stream(payload);
                stream.setVersion(dataStreamVersion);
                result = QVariant(metaType);
                if (!metaType.load(stream, result.data()) || stream.status() != QDataStream::Ok) {
                    decoder.setFailed();
                }
                break;
            }
            default:
                decoder.setFailed();
                break;
        }
        return decoder.fail() ? QVariant() : result;
    }

    void QCodec::writeString(Encoder &encoder, const QString &text) {
        encoder.writeBytes(ArrayView<char>(reinterpret_cast<const char *>(text.utf16()),
                                           size_t(text.size()) * sizeof(char16_t)));
    }

    QString QCodec::readString(Decoder &decoder) {
        const auto bytes = decoder.readBytes();
        if (decoder.fail() || bytes.size() % sizeof(char16_t) != 0) {
            decoder.setFailed();
            return {};
        }
        QString text(qsizetype(bytes.size() / sizeof(char16_t)), Qt::Uninitialized);
        std::memcpy(text.data(), bytes.data(), bytes.size());
        return text;
    }

}
