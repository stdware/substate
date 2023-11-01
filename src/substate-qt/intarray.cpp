#include "intarray.h"

#include <substate/bytesnode.h>

#include "entity_p.h"

namespace Substate {

    class IntArrayBasePrivate : public EntityPrivate {
    public:
        IntArrayBasePrivate(Node *node, int type_size) : EntityPrivate(node), type_size(type_size) {
        }

        ~IntArrayBasePrivate() {
        }

        void init() {
        }

        void notified(Notification *n) {
            switch (n->type()) {
                case Notification::ActionTriggered: {
                    auto n2 = static_cast<ActionNotification *>(n);
                    auto a = n2->action();
                    switch (a->type()) {
                        case Action::BytesReplace: {
                            auto a2 = static_cast<BytesAction *>(a);
                            sendReplace(a2->index(), a2->bytes(), a2->oldBytes());
                            break;
                        }
                        case Action::BytesInsert: {
                            auto a2 = static_cast<BytesAction *>(a);
                            sendInsert(a2->index(), a2->bytes());
                            break;
                        }
                        case Action::BytesRemove: {
                            auto a2 = static_cast<BytesAction *>(a);
                            sendRemove(a2->index(), a2->bytes());
                            break;
                        }
                    }
                    break;
                }
                default:
                    break;
            }
        }

        virtual void sendReplace(int index, const ByteArray &bytes, const ByteArray &oldBytes) = 0;
        virtual void sendInsert(int index, const ByteArray &bytes) = 0;
        virtual void sendRemove(int index, const ByteArray &bytes) = 0;

        int type_size;
    };

    IntArrayBase::~IntArrayBase() {
    }

    int IntArrayBase::sizeImpl() const {
        Q_D(const IntArrayBase);
        return static_cast<BytesNode *>(d->internalData())->size() / int(d->type_size);
    }

    const char *IntArrayBase::valuesImpl() const {
        Q_D(const IntArrayBase);
        return static_cast<BytesNode *>(d->internalData())->data();
    }

    void IntArrayBase::replaceImpl(int index, const char *data, int size) {
        Q_D(IntArrayBase);
        static_cast<BytesNode *>(d->internalData())
            ->replace(index * d->type_size, data, size * d->type_size);
    }

    void IntArrayBase::insertImpl(int index, const char *data, int size) {
        Q_D(IntArrayBase);
        static_cast<BytesNode *>(d->internalData())
            ->insert(index * d->type_size, data, size * d->type_size);
    }

    void IntArrayBase::removeImpl(int index, int size) {
        Q_D(IntArrayBase);
        static_cast<BytesNode *>(d->internalData())
            ->remove(index * d->type_size, size * d->type_size);
    }

    IntArrayBase::IntArrayBase(IntArrayBasePrivate &d, QObject *parent) : Entity(d, parent) {
        d.init();
    }

    //===========================================================================
    // Int8
    class Int8ArrayPrivate : public IntArrayBasePrivate {
        Q_DECLARE_PUBLIC(Int8Array)
    public:
        Int8ArrayPrivate(Node *node) : IntArrayBasePrivate(node, 8) {
        }
        ~Int8ArrayPrivate() {
        }

        void sendReplace(int index, const ByteArray &bytes, const ByteArray &oldBytes) override {
            Q_Q(Int8Array);
            Q_UNUSED(oldBytes)
            auto data = reinterpret_cast<const qint8 *>(bytes.data());
            emit q->replaced(index / type_size, {data, data + int(bytes.size() / type_size)});
        }
        void sendInsert(int index, const ByteArray &bytes) override {
            Q_Q(Int8Array);
            auto data = reinterpret_cast<const qint8 *>(bytes.data());
            emit q->inserted(index / type_size, {data, data + int(bytes.size() / type_size)});
        }
        void sendRemove(int index, const ByteArray &bytes) override {
            Q_Q(Int8Array);
            auto data = reinterpret_cast<const qint8 *>(bytes.data());
            emit q->removed(index / type_size, {data, data + int(bytes.size() / type_size)});
        }
    };

    Int8Array::Int8Array(QObject *parent)
        : IntArrayBase(*new Int8ArrayPrivate(new BytesNode()), parent) {
    }

    Int8Array::Int8Array(Node *node, QObject *parent)
        : IntArrayBase(*new Int8ArrayPrivate(node), parent) {
    }

    Int8Array::~Int8Array() {
    }

    QVector<qint8> Int8Array::mid(int index, int size) const {
        if (index < 0 || index >= this->size()) {
            return {};
        }
        size = qMin(this->size() - index, size);
        auto data = reinterpret_cast<const qint8 *>(valuesImpl()) + index;
        return {data, data + size};
    }

    QVector<qint8> Int8Array::values() const {
        auto data = reinterpret_cast<const qint8 *>(valuesImpl());
        return {data, data + size()};
    }

    void Int8Array::replace(int index, const QVector<qint8> &values) {
        replaceImpl(index, reinterpret_cast<const char *>(values.constData()), values.size());
    }

    void Int8Array::insert(int index, const QVector<qint8> &values) {
        insertImpl(index, reinterpret_cast<const char *>(values.constData()), values.size());
    }

    void Int8Array::remove(int index, int count) {
        removeImpl(index, count);
    }
    //===========================================================================

    //===========================================================================
    // UInt8
    class UInt8ArrayPrivate : public IntArrayBasePrivate {
        Q_DECLARE_PUBLIC(UInt8Array)
    public:
        UInt8ArrayPrivate(Node *node) : IntArrayBasePrivate(node, 8) {
        }
        ~UInt8ArrayPrivate() {
        }

        void sendReplace(int index, const ByteArray &bytes, const ByteArray &oldBytes) override {
            Q_Q(UInt8Array);
            Q_UNUSED(oldBytes)
            auto data = reinterpret_cast<const quint8 *>(bytes.data());
            emit q->replaced(index / type_size, {data, data + int(bytes.size() / type_size)});
        }
        void sendInsert(int index, const ByteArray &bytes) override {
            Q_Q(UInt8Array);
            auto data = reinterpret_cast<const quint8 *>(bytes.data());
            emit q->inserted(index / type_size, {data, data + int(bytes.size() / type_size)});
        }
        void sendRemove(int index, const ByteArray &bytes) override {
            Q_Q(UInt8Array);
            auto data = reinterpret_cast<const quint8 *>(bytes.data());
            emit q->removed(index / type_size, {data, data + int(bytes.size() / type_size)});
        }
    };

    UInt8Array::UInt8Array(QObject *parent)
        : IntArrayBase(*new UInt8ArrayPrivate(new BytesNode()), parent) {
    }

    UInt8Array::UInt8Array(Node *node, QObject *parent)
        : IntArrayBase(*new UInt8ArrayPrivate(node), parent) {
    }

    UInt8Array::~UInt8Array() {
    }

    QVector<quint8> UInt8Array::mid(int index, int size) const {
        if (index < 0 || index >= this->size()) {
            return {};
        }
        size = qMin(this->size() - index, size);
        auto data = reinterpret_cast<const quint8 *>(valuesImpl()) + index;
        return {data, data + size};
    }

    QVector<quint8> UInt8Array::values() const {
        auto data = reinterpret_cast<const quint8 *>(valuesImpl());
        return {data, data + size()};
    }

    void UInt8Array::replace(int index, const QVector<quint8> &values) {
        replaceImpl(index, reinterpret_cast<const char *>(values.constData()), values.size());
    }

    void UInt8Array::insert(int index, const QVector<quint8> &values) {
        insertImpl(index, reinterpret_cast<const char *>(values.constData()), values.size());
    }

    void UInt8Array::remove(int index, int count) {
        removeImpl(index, count);
    }
    //===========================================================================


    //===========================================================================
    // Int16
    class Int16ArrayPrivate : public IntArrayBasePrivate {
        Q_DECLARE_PUBLIC(Int16Array)
    public:
        Int16ArrayPrivate(Node *node) : IntArrayBasePrivate(node, 16) {
        }
        ~Int16ArrayPrivate() {
        }

        void sendReplace(int index, const ByteArray &bytes, const ByteArray &oldBytes) override {
            Q_Q(Int16Array);
            Q_UNUSED(oldBytes)
            auto data = reinterpret_cast<const qint16 *>(bytes.data());
            emit q->replaced(index / type_size, {data, data + int(bytes.size() / type_size)});
        }
        void sendInsert(int index, const ByteArray &bytes) override {
            Q_Q(Int16Array);
            auto data = reinterpret_cast<const qint16 *>(bytes.data());
            emit q->inserted(index / type_size, {data, data + int(bytes.size() / type_size)});
        }
        void sendRemove(int index, const ByteArray &bytes) override {
            Q_Q(Int16Array);
            auto data = reinterpret_cast<const qint16 *>(bytes.data());
            emit q->removed(index / type_size, {data, data + int(bytes.size() / type_size)});
        }
    };

    Int16Array::Int16Array(QObject *parent)
        : IntArrayBase(*new Int16ArrayPrivate(new BytesNode()), parent) {
    }

    Int16Array::Int16Array(Node *node, QObject *parent)
        : IntArrayBase(*new Int16ArrayPrivate(node), parent) {
    }

    Int16Array::~Int16Array() {
    }

    QVector<qint16> Int16Array::mid(int index, int size) const {
        if (index < 0 || index >= this->size()) {
            return {};
        }
        size = qMin(this->size() - index, size);
        auto data = reinterpret_cast<const qint16 *>(valuesImpl()) + index;
        return {data, data + size};
    }

    QVector<qint16> Int16Array::values() const {
        auto data = reinterpret_cast<const qint16 *>(valuesImpl());
        return {data, data + size()};
    }

    void Int16Array::replace(int index, const QVector<qint16> &values) {
        replaceImpl(index, reinterpret_cast<const char *>(values.constData()), values.size());
    }

    void Int16Array::insert(int index, const QVector<qint16> &values) {
        insertImpl(index, reinterpret_cast<const char *>(values.constData()), values.size());
    }

    void Int16Array::remove(int index, int count) {
        removeImpl(index, count);
    }
    //===========================================================================

    //===========================================================================
    // UInt16
    class UInt16ArrayPrivate : public IntArrayBasePrivate {
        Q_DECLARE_PUBLIC(UInt16Array)
    public:
        UInt16ArrayPrivate(Node *node) : IntArrayBasePrivate(node, 16) {
        }
        ~UInt16ArrayPrivate() {
        }

        void sendReplace(int index, const ByteArray &bytes, const ByteArray &oldBytes) override {
            Q_Q(UInt16Array);
            Q_UNUSED(oldBytes)
            auto data = reinterpret_cast<const quint16 *>(bytes.data());
            emit q->replaced(index / type_size, {data, data + int(bytes.size() / type_size)});
        }
        void sendInsert(int index, const ByteArray &bytes) override {
            Q_Q(UInt16Array);
            auto data = reinterpret_cast<const quint16 *>(bytes.data());
            emit q->inserted(index / type_size, {data, data + int(bytes.size() / type_size)});
        }
        void sendRemove(int index, const ByteArray &bytes) override {
            Q_Q(UInt16Array);
            auto data = reinterpret_cast<const quint16 *>(bytes.data());
            emit q->removed(index / type_size, {data, data + int(bytes.size() / type_size)});
        }
    };

    UInt16Array::UInt16Array(QObject *parent)
        : IntArrayBase(*new UInt16ArrayPrivate(new BytesNode()), parent) {
    }

    UInt16Array::UInt16Array(Node *node, QObject *parent)
        : IntArrayBase(*new UInt16ArrayPrivate(node), parent) {
    }

    UInt16Array::~UInt16Array() {
    }

    QVector<quint16> UInt16Array::mid(int index, int size) const {
        if (index < 0 || index >= this->size()) {
            return {};
        }
        size = qMin(this->size() - index, size);
        auto data = reinterpret_cast<const quint16 *>(valuesImpl()) + index;
        return {data, data + size};
    }

    QVector<quint16> UInt16Array::values() const {
        auto data = reinterpret_cast<const quint16 *>(valuesImpl());
        return {data, data + size()};
    }

    void UInt16Array::replace(int index, const QVector<quint16> &values) {
        replaceImpl(index, reinterpret_cast<const char *>(values.constData()), values.size());
    }

    void UInt16Array::insert(int index, const QVector<quint16> &values) {
        insertImpl(index, reinterpret_cast<const char *>(values.constData()), values.size());
    }

    void UInt16Array::remove(int index, int count) {
        removeImpl(index, count);
    }
    //===========================================================================

    //===========================================================================
    // Int32
    class Int32ArrayPrivate : public IntArrayBasePrivate {
        Q_DECLARE_PUBLIC(Int32Array)
    public:
        Int32ArrayPrivate(Node *node) : IntArrayBasePrivate(node, 32) {
        }
        ~Int32ArrayPrivate() {
        }

        void sendReplace(int index, const ByteArray &bytes, const ByteArray &oldBytes) override {
            Q_Q(Int32Array);
            Q_UNUSED(oldBytes)
            auto data = reinterpret_cast<const qint32 *>(bytes.data());
            emit q->replaced(index / type_size, {data, data + int(bytes.size() / type_size)});
        }
        void sendInsert(int index, const ByteArray &bytes) override {
            Q_Q(Int32Array);
            auto data = reinterpret_cast<const qint32 *>(bytes.data());
            emit q->inserted(index / type_size, {data, data + int(bytes.size() / type_size)});
        }
        void sendRemove(int index, const ByteArray &bytes) override {
            Q_Q(Int32Array);
            auto data = reinterpret_cast<const qint32 *>(bytes.data());
            emit q->removed(index / type_size, {data, data + int(bytes.size() / type_size)});
        }
    };

    Int32Array::Int32Array(QObject *parent)
        : IntArrayBase(*new Int32ArrayPrivate(new BytesNode()), parent) {
    }

    Int32Array::Int32Array(Node *node, QObject *parent)
        : IntArrayBase(*new Int32ArrayPrivate(node), parent) {
    }

    Int32Array::~Int32Array() {
    }

    QVector<qint32> Int32Array::mid(int index, int size) const {
        if (index < 0 || index >= this->size()) {
            return {};
        }
        size = qMin(this->size() - index, size);
        auto data = reinterpret_cast<const qint32 *>(valuesImpl()) + index;
        return {data, data + size};
    }

    QVector<qint32> Int32Array::values() const {
        auto data = reinterpret_cast<const qint32 *>(valuesImpl());
        return {data, data + size()};
    }

    void Int32Array::replace(int index, const QVector<qint32> &values) {
        replaceImpl(index, reinterpret_cast<const char *>(values.constData()), values.size());
    }

    void Int32Array::insert(int index, const QVector<qint32> &values) {
        insertImpl(index, reinterpret_cast<const char *>(values.constData()), values.size());
    }

    void Int32Array::remove(int index, int count) {
        removeImpl(index, count);
    }
    //===========================================================================

    //===========================================================================
    // UInt32
    class UInt32ArrayPrivate : public IntArrayBasePrivate {
        Q_DECLARE_PUBLIC(UInt32Array)
    public:
        UInt32ArrayPrivate(Node *node) : IntArrayBasePrivate(node, 32) {
        }
        ~UInt32ArrayPrivate() {
        }

        void sendReplace(int index, const ByteArray &bytes, const ByteArray &oldBytes) override {
            Q_Q(UInt32Array);
            Q_UNUSED(oldBytes)
            auto data = reinterpret_cast<const quint32 *>(bytes.data());
            emit q->replaced(index / type_size, {data, data + int(bytes.size() / type_size)});
        }
        void sendInsert(int index, const ByteArray &bytes) override {
            Q_Q(UInt32Array);
            auto data = reinterpret_cast<const quint32 *>(bytes.data());
            emit q->inserted(index / type_size, {data, data + int(bytes.size() / type_size)});
        }
        void sendRemove(int index, const ByteArray &bytes) override {
            Q_Q(UInt32Array);
            auto data = reinterpret_cast<const quint32 *>(bytes.data());
            emit q->removed(index / type_size, {data, data + int(bytes.size() / type_size)});
        }
    };

    UInt32Array::UInt32Array(QObject *parent)
        : IntArrayBase(*new UInt32ArrayPrivate(new BytesNode()), parent) {
    }

    UInt32Array::UInt32Array(Node *node, QObject *parent)
        : IntArrayBase(*new UInt32ArrayPrivate(node), parent) {
    }

    UInt32Array::~UInt32Array() {
    }

    QVector<quint32> UInt32Array::mid(int index, int size) const {
        if (index < 0 || index >= this->size()) {
            return {};
        }
        size = qMin(this->size() - index, size);
        auto data = reinterpret_cast<const quint32 *>(valuesImpl()) + index;
        return {data, data + size};
    }

    QVector<quint32> UInt32Array::values() const {
        auto data = reinterpret_cast<const quint32 *>(valuesImpl());
        return {data, data + size()};
    }

    void UInt32Array::replace(int index, const QVector<quint32> &values) {
        replaceImpl(index, reinterpret_cast<const char *>(values.constData()), values.size());
    }

    void UInt32Array::insert(int index, const QVector<quint32> &values) {
        insertImpl(index, reinterpret_cast<const char *>(values.constData()), values.size());
    }

    void UInt32Array::remove(int index, int count) {
        removeImpl(index, count);
    }
    //===========================================================================

    //===========================================================================
    // Int64
    class Int64ArrayPrivate : public IntArrayBasePrivate {
        Q_DECLARE_PUBLIC(Int64Array)
    public:
        Int64ArrayPrivate(Node *node) : IntArrayBasePrivate(node, 64) {
        }
        ~Int64ArrayPrivate() {
        }

        void sendReplace(int index, const ByteArray &bytes, const ByteArray &oldBytes) override {
            Q_Q(Int64Array);
            Q_UNUSED(oldBytes)
            auto data = reinterpret_cast<const qint64 *>(bytes.data());
            emit q->replaced(index / type_size, {data, data + int(bytes.size() / type_size)});
        }
        void sendInsert(int index, const ByteArray &bytes) override {
            Q_Q(Int64Array);
            auto data = reinterpret_cast<const qint64 *>(bytes.data());
            emit q->inserted(index / type_size, {data, data + int(bytes.size() / type_size)});
        }
        void sendRemove(int index, const ByteArray &bytes) override {
            Q_Q(Int64Array);
            auto data = reinterpret_cast<const qint64 *>(bytes.data());
            emit q->removed(index / type_size, {data, data + int(bytes.size() / type_size)});
        }
    };

    Int64Array::Int64Array(QObject *parent)
        : IntArrayBase(*new Int64ArrayPrivate(new BytesNode()), parent) {
    }

    Int64Array::Int64Array(Node *node, QObject *parent)
        : IntArrayBase(*new Int64ArrayPrivate(node), parent) {
    }

    Int64Array::~Int64Array() {
    }

    QVector<qint64> Int64Array::mid(int index, int size) const {
        if (index < 0 || index >= this->size()) {
            return {};
        }
        size = qMin(this->size() - index, size);
        auto data = reinterpret_cast<const qint64 *>(valuesImpl()) + index;
        return {data, data + size};
    }

    QVector<qint64> Int64Array::values() const {
        auto data = reinterpret_cast<const qint64 *>(valuesImpl());
        return {data, data + size()};
    }

    void Int64Array::replace(int index, const QVector<qint64> &values) {
        replaceImpl(index, reinterpret_cast<const char *>(values.constData()), values.size());
    }

    void Int64Array::insert(int index, const QVector<qint64> &values) {
        insertImpl(index, reinterpret_cast<const char *>(values.constData()), values.size());
    }

    void Int64Array::remove(int index, int count) {
        removeImpl(index, count);
    }
    //===========================================================================

    //===========================================================================
    // UInt64
    class UInt64ArrayPrivate : public IntArrayBasePrivate {
        Q_DECLARE_PUBLIC(UInt64Array)
    public:
        UInt64ArrayPrivate(Node *node) : IntArrayBasePrivate(node, 64) {
        }
        ~UInt64ArrayPrivate() {
        }

        void sendReplace(int index, const ByteArray &bytes, const ByteArray &oldBytes) override {
            Q_Q(UInt64Array);
            Q_UNUSED(oldBytes)
            auto data = reinterpret_cast<const quint64 *>(bytes.data());
            emit q->replaced(index / type_size, {data, data + int(bytes.size() / type_size)});
        }
        void sendInsert(int index, const ByteArray &bytes) override {
            Q_Q(UInt64Array);
            auto data = reinterpret_cast<const quint64 *>(bytes.data());
            emit q->inserted(index / type_size, {data, data + int(bytes.size() / type_size)});
        }
        void sendRemove(int index, const ByteArray &bytes) override {
            Q_Q(UInt64Array);
            auto data = reinterpret_cast<const quint64 *>(bytes.data());
            emit q->removed(index / type_size, {data, data + int(bytes.size() / type_size)});
        }
    };

    UInt64Array::UInt64Array(QObject *parent)
        : IntArrayBase(*new UInt64ArrayPrivate(new BytesNode()), parent) {
    }

    UInt64Array::UInt64Array(Node *node, QObject *parent)
        : IntArrayBase(*new UInt64ArrayPrivate(node), parent) {
    }

    UInt64Array::~UInt64Array() {
    }

    QVector<quint64> UInt64Array::mid(int index, int size) const {
        if (index < 0 || index >= this->size()) {
            return {};
        }
        size = qMin(this->size() - index, size);
        auto data = reinterpret_cast<const quint64 *>(valuesImpl()) + index;
        return {data, data + size};
    }

    QVector<quint64> UInt64Array::values() const {
        auto data = reinterpret_cast<const quint64 *>(valuesImpl());
        return {data, data + size()};
    }

    void UInt64Array::replace(int index, const QVector<quint64> &values) {
        replaceImpl(index, reinterpret_cast<const char *>(values.constData()), values.size());
    }

    void UInt64Array::insert(int index, const QVector<quint64> &values) {
        insertImpl(index, reinterpret_cast<const char *>(values.constData()), values.size());
    }

    void UInt64Array::remove(int index, int count) {
        removeImpl(index, count);
    }
    //===========================================================================

    
    //===========================================================================
    // Double
    class DoubleArrayPrivate : public IntArrayBasePrivate {
        Q_DECLARE_PUBLIC(DoubleArray)
    public:
        DoubleArrayPrivate(Node *node) : IntArrayBasePrivate(node, 64) {
        }
        ~DoubleArrayPrivate() {
        }

        void sendReplace(int index, const ByteArray &bytes, const ByteArray &oldBytes) override {
            Q_Q(DoubleArray);
            Q_UNUSED(oldBytes)
            auto data = reinterpret_cast<const double *>(bytes.data());
            emit q->replaced(index / type_size, {data, data + int(bytes.size() / type_size)});
        }
        void sendInsert(int index, const ByteArray &bytes) override {
            Q_Q(DoubleArray);
            auto data = reinterpret_cast<const double *>(bytes.data());
            emit q->inserted(index / type_size, {data, data + int(bytes.size() / type_size)});
        }
        void sendRemove(int index, const ByteArray &bytes) override {
            Q_Q(DoubleArray);
            auto data = reinterpret_cast<const double *>(bytes.data());
            emit q->removed(index / type_size, {data, data + int(bytes.size() / type_size)});
        }
    };

    DoubleArray::DoubleArray(QObject *parent)
        : IntArrayBase(*new DoubleArrayPrivate(new BytesNode()), parent) {
    }

    DoubleArray::DoubleArray(Node *node, QObject *parent)
        : IntArrayBase(*new DoubleArrayPrivate(node), parent) {
    }

    DoubleArray::~DoubleArray() {
    }

    QVector<double> DoubleArray::mid(int index, int size) const {
        if (index < 0 || index >= this->size()) {
            return {};
        }
        size = qMin(this->size() - index, size);
        auto data = reinterpret_cast<const double *>(valuesImpl()) + index;
        return {data, data + size};
    }

    QVector<double> DoubleArray::values() const {
        auto data = reinterpret_cast<const double *>(valuesImpl());
        return {data, data + size()};
    }

    void DoubleArray::replace(int index, const QVector<double> &values) {
        replaceImpl(index, reinterpret_cast<const char *>(values.constData()), values.size());
    }

    void DoubleArray::insert(int index, const QVector<double> &values) {
        insertImpl(index, reinterpret_cast<const char *>(values.constData()), values.size());
    }

    void DoubleArray::remove(int index, int count) {
        removeImpl(index, count);
    }
    //===========================================================================

}