#ifndef ARRAYENTITY_H
#define ARRAYENTITY_H

#include <QObject>

#include <qsubstate/entity.h>

namespace Substate {

    class ArrayEntityBasePrivate;

    class QSUBSTATE_EXPORT ArrayEntityBase : public Entity {
        Q_OBJECT
        Q_DECLARE_PRIVATE(ArrayEntityBase)
    public:
        ~ArrayEntityBase();

    protected:
        int sizeImpl() const;
        const char *valuesImpl() const;
        void replaceImpl(int index, const char *data, int size);
        void insertImpl(int index, const char *data, int size);
        void removeImpl(int index, int size);

    protected:
        virtual void sendReplace(int index, const char *bytes, int bytesSize) = 0;
        virtual void sendInsert(int index, const char *bytes, int bytesSize) = 0;
        virtual void sendRemove(int index, const char *bytes, int bytesSize) = 0;

    protected:
        ArrayEntityBase(Node *node, QObject *parent = nullptr);
        ArrayEntityBase(ArrayEntityBasePrivate &d, QObject *parent = nullptr);
    };

    template <class Container, class T>
    class ArrayEntityHelper {
    public:
        int size() const {
            return static_cast<const Container *>(this)->sizeImpl() / N;
        }

        const T *data() const {
            return reinterpret_cast<const T *>(static_cast<const Container *>(this)->valuesImpl());
        }

        QVector<T> mid(int index, int len) const {
            auto data =
                reinterpret_cast<const T *>(static_cast<const Container *>(this)->valuesImpl()) +
                index;
            return {data, data + qMin(this->size() - index, len)};
        }

        QVector<T> values() const {
            auto data =
                reinterpret_cast<const T *>(static_cast<const Container *>(this)->valuesImpl());
            return {data, data + size()};
        }

        void replace(int index, const QVector<T> &values) {
            static_cast<Container *>(this)->replaceImpl(
                index * N, reinterpret_cast<const char *>(values.constData()), values.size() * N);
        }

        void insert(int index, const QVector<T> &values) {
            static_cast<Container *>(this)->insertImpl(
                index * N, reinterpret_cast<const char *>(values.constData()), values.size() * N);
        }

        void remove(int index, int len) {
            static_cast<Container *>(this)->removeImpl(index * N, len * N);
        }

    private:
        QSUBSTATE_INLINE void sendReplaceHelper(int index, const char *bytes, int bytesSize) {
            auto data = reinterpret_cast<const T *>(bytes);
            Q_EMIT static_cast<Container *>(this)->replaced(index / N,
                                                            {data, data + int(bytesSize / N)});
        }
        QSUBSTATE_INLINE void sendInsertHelper(int index, const char *bytes, int bytesSize) {
            auto data = reinterpret_cast<const T *>(bytes);
            Q_EMIT static_cast<Container *>(this)->inserted(index / N,
                                                            {data, data + int(bytesSize / N)});
        }
        QSUBSTATE_INLINE void sendRemoveHelper(int index, const char *bytes, int bytesSize) {
            Q_UNUSED(bytes)
            Q_EMIT static_cast<Container *>(this)->removed(index / N, int(bytesSize / N));
        }

    protected:
        friend Container;

        static constexpr size_t N = sizeof(T);
    };

#define Q_SUBSTATE_DECLARE_ARRAY(Container, T)                                                     \
    friend class ArrayEntityHelper<Container, T>;                                                  \
                                                                                                   \
Q_SIGNALS:                                                                                         \
    void replaced(int index, const QVector<T> &values);                                            \
    void inserted(int index, const QVector<T> &values);                                            \
    void removed(int index, int size);                                                             \
                                                                                                   \
protected:                                                                                         \
    void sendReplace(int index, const char *bytes, int bytesSize) override {                       \
        sendReplaceHelper(index, bytes, bytesSize);                                                \
    }                                                                                              \
    void sendInsert(int index, const char *bytes, int bytesSize) override {                        \
        sendInsertHelper(index, bytes, bytesSize);                                                 \
    }                                                                                              \
    void sendRemove(int index, const char *bytes, int bytesSize) override {                        \
        sendRemoveHelper(index, bytes, bytesSize);                                                 \
    }

    class QSUBSTATE_EXPORT Int8ArrayEntity : public ArrayEntityBase,
                                             public ArrayEntityHelper<Int8ArrayEntity, qint8> {
        Q_OBJECT
        Q_SUBSTATE_DECLARE_ARRAY(Int8ArrayEntity, qint8)
    public:
        inline Int8ArrayEntity(QObject *parent = nullptr) : ArrayEntityBase(nullptr, parent) {
        }

    protected:
        inline Int8ArrayEntity(Node *node, QObject *parent = nullptr)
            : ArrayEntityBase(node, parent) {
        }
    };

    class QSUBSTATE_EXPORT UInt8ArrayEntity : public ArrayEntityBase,
                                              public ArrayEntityHelper<UInt8ArrayEntity, quint8> {
        Q_OBJECT
        Q_SUBSTATE_DECLARE_ARRAY(UInt8ArrayEntity, quint8)
    public:
        inline UInt8ArrayEntity(QObject *parent = nullptr) : ArrayEntityBase(nullptr, parent) {
        }

    protected:
        inline UInt8ArrayEntity(Node *node, QObject *parent = nullptr)
            : ArrayEntityBase(node, parent) {
        }
    };

    class QSUBSTATE_EXPORT Int16ArrayEntity : public ArrayEntityBase,
                                              public ArrayEntityHelper<Int16ArrayEntity, qint16> {
        Q_OBJECT
        Q_SUBSTATE_DECLARE_ARRAY(Int16ArrayEntity, qint16)
    public:
        inline Int16ArrayEntity(QObject *parent = nullptr) : ArrayEntityBase(nullptr, parent) {
        }

    protected:
        inline Int16ArrayEntity(Node *node, QObject *parent = nullptr)
            : ArrayEntityBase(node, parent) {
        }
    };

    class QSUBSTATE_EXPORT UInt16ArrayEntity
        : public ArrayEntityBase,
          public ArrayEntityHelper<UInt16ArrayEntity, quint16> {
        Q_OBJECT
        Q_SUBSTATE_DECLARE_ARRAY(UInt16ArrayEntity, quint16)
    public:
        inline UInt16ArrayEntity(QObject *parent = nullptr) : ArrayEntityBase(nullptr, parent) {
        }

    protected:
        inline UInt16ArrayEntity(Node *node, QObject *parent = nullptr)
            : ArrayEntityBase(node, parent) {
        }
    };

    class QSUBSTATE_EXPORT Int32ArrayEntity : public ArrayEntityBase,
                                              public ArrayEntityHelper<Int32ArrayEntity, qint32> {
        Q_OBJECT
        Q_SUBSTATE_DECLARE_ARRAY(Int32ArrayEntity, qint32)
    public:
        inline Int32ArrayEntity(QObject *parent = nullptr) : ArrayEntityBase(nullptr, parent) {
        }

    protected:
        inline Int32ArrayEntity(Node *node, QObject *parent = nullptr)
            : ArrayEntityBase(node, parent) {
        }
    };

    class QSUBSTATE_EXPORT UInt32ArrayEntity
        : public ArrayEntityBase,
          public ArrayEntityHelper<UInt32ArrayEntity, quint32> {
        Q_OBJECT
        Q_SUBSTATE_DECLARE_ARRAY(UInt32ArrayEntity, quint32)
    public:
        inline UInt32ArrayEntity(QObject *parent = nullptr) : ArrayEntityBase(nullptr, parent) {
        }

    protected:
        inline UInt32ArrayEntity(Node *node, QObject *parent = nullptr)
            : ArrayEntityBase(node, parent) {
        }
    };

    class QSUBSTATE_EXPORT Int64ArrayEntity : public ArrayEntityBase,
                                              public ArrayEntityHelper<Int64ArrayEntity, qint64> {
        Q_OBJECT
        Q_SUBSTATE_DECLARE_ARRAY(Int64ArrayEntity, qint64)
    public:
        inline Int64ArrayEntity(QObject *parent = nullptr) : ArrayEntityBase(nullptr, parent) {
        }

    protected:
        inline Int64ArrayEntity(Node *node, QObject *parent = nullptr)
            : ArrayEntityBase(node, parent) {
        }
    };

    class QSUBSTATE_EXPORT UInt64ArrayEntity
        : public ArrayEntityBase,
          public ArrayEntityHelper<UInt64ArrayEntity, quint64> {
        Q_OBJECT
        Q_SUBSTATE_DECLARE_ARRAY(UInt64ArrayEntity, quint64)
    public:
        inline UInt64ArrayEntity(QObject *parent = nullptr) : ArrayEntityBase(nullptr, parent) {
        }

    protected:
        inline UInt64ArrayEntity(Node *node, QObject *parent = nullptr)
            : ArrayEntityBase(node, parent) {
        }
    };

}

#endif // ARRAYENTITY_H
