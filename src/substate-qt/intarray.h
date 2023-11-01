#ifndef INTARRAY_H
#define INTARRAY_H

#include <QObject>

#include <qsubstate/entity.h>

namespace Substate {

    class IntArrayBasePrivate;

    class QSUBSTATE_EXPORT IntArrayBase : public Entity {
        Q_OBJECT
        Q_DECLARE_PRIVATE(IntArrayBase)
    public:
        ~IntArrayBase();

    protected:
        int sizeImpl() const;
        const char *valuesImpl() const;
        void replaceImpl(int index, const char *data, int size);
        void insertImpl(int index, const char *data, int size);
        void removeImpl(int index, int size);

    protected:
        IntArrayBase(IntArrayBasePrivate &d, QObject *parent = nullptr);
    };

    class Int8ArrayPrivate;

    class QSUBSTATE_EXPORT Int8Array : public IntArrayBase {
        Q_OBJECT
        Q_DECLARE_PRIVATE(Int8Array)
    public:
        Int8Array(QObject *parent = nullptr);
        Int8Array(Node *node, QObject *parent = nullptr);
        ~Int8Array();

    public:
        inline int size() const;
        inline const qint8 *data() const;
        QVector<qint8> mid(int index, int size) const;
        QVector<qint8> values() const;
        void replace(int index, const QVector<qint8> &values);
        void insert(int index, const QVector<qint8> &values);
        void remove(int index, int count);

    Q_SIGNALS:
        void replaced(int index, const QVector<qint8> &values);
        void inserted(int index, const QVector<qint8> &values);
        void removed(int index, const QVector<qint8> &values);
    };

    inline int Int8Array::size() const {
        return sizeImpl();
    }

    inline const qint8 *Int8Array::data() const {
        return reinterpret_cast<const qint8 *>(valuesImpl());
    }

    class UInt8ArrayPrivate;

    class QSUBSTATE_EXPORT UInt8Array : public IntArrayBase {
        Q_OBJECT
        Q_DECLARE_PRIVATE(UInt8Array)
    public:
        UInt8Array(QObject *parent = nullptr);
        UInt8Array(Node *node, QObject *parent = nullptr);
        ~UInt8Array();

    public:
        inline int size() const;
        inline const quint8 *data() const;
        QVector<quint8> mid(int index, int size) const;
        QVector<quint8> values() const;
        void replace(int index, const QVector<quint8> &values);
        void insert(int index, const QVector<quint8> &values);
        void remove(int index, int count);

    Q_SIGNALS:
        void replaced(int index, const QVector<quint8> &values);
        void inserted(int index, const QVector<quint8> &values);
        void removed(int index, const QVector<quint8> &values);
    };

    inline int UInt8Array::size() const {
        return sizeImpl();
    }

    inline const quint8 *UInt8Array::data() const {
        return reinterpret_cast<const quint8 *>(valuesImpl());
    }

    class Int16ArrayPrivate;

    class QSUBSTATE_EXPORT Int16Array : public IntArrayBase {
        Q_OBJECT
        Q_DECLARE_PRIVATE(Int16Array)
    public:
        Int16Array(QObject *parent = nullptr);
        Int16Array(Node *node, QObject *parent = nullptr);
        ~Int16Array();

    public:
        inline int size() const;
        inline const qint16 *data() const;
        QVector<qint16> mid(int index, int size) const;
        QVector<qint16> values() const;
        void replace(int index, const QVector<qint16> &values);
        void insert(int index, const QVector<qint16> &values);
        void remove(int index, int count);

    Q_SIGNALS:
        void replaced(int index, const QVector<qint16> &values);
        void inserted(int index, const QVector<qint16> &values);
        void removed(int index, const QVector<qint16> &values);
    };

    inline int Int16Array::size() const {
        return sizeImpl();
    }

    inline const qint16 *Int16Array::data() const {
        return reinterpret_cast<const qint16 *>(valuesImpl());
    }

    class UInt16ArrayPrivate;

    class QSUBSTATE_EXPORT UInt16Array : public IntArrayBase {
        Q_OBJECT
        Q_DECLARE_PRIVATE(UInt16Array)
    public:
        UInt16Array(QObject *parent = nullptr);
        UInt16Array(Node *node, QObject *parent = nullptr);
        ~UInt16Array();

    public:
        inline int size() const;
        inline const quint16 *data() const;
        QVector<quint16> mid(int index, int size) const;
        QVector<quint16> values() const;
        void replace(int index, const QVector<quint16> &values);
        void insert(int index, const QVector<quint16> &values);
        void remove(int index, int count);

    Q_SIGNALS:
        void replaced(int index, const QVector<quint16> &values);
        void inserted(int index, const QVector<quint16> &values);
        void removed(int index, const QVector<quint16> &values);
    };

    inline int UInt16Array::size() const {
        return sizeImpl();
    }

    inline const quint16 *UInt16Array::data() const {
        return reinterpret_cast<const quint16 *>(valuesImpl());
    }

    class Int32ArrayPrivate;

    class QSUBSTATE_EXPORT Int32Array : public IntArrayBase {
        Q_OBJECT
        Q_DECLARE_PRIVATE(Int32Array)
    public:
        Int32Array(QObject *parent = nullptr);
        Int32Array(Node *node, QObject *parent = nullptr);
        ~Int32Array();

    public:
        inline int size() const;
        inline const qint32 *data() const;
        QVector<qint32> mid(int index, int size) const;
        QVector<qint32> values() const;
        void replace(int index, const QVector<qint32> &values);
        void insert(int index, const QVector<qint32> &values);
        void remove(int index, int count);

    Q_SIGNALS:
        void replaced(int index, const QVector<qint32> &values);
        void inserted(int index, const QVector<qint32> &values);
        void removed(int index, const QVector<qint32> &values);
    };

    inline int Int32Array::size() const {
        return sizeImpl();
    }

    inline const qint32 *Int32Array::data() const {
        return reinterpret_cast<const qint32 *>(valuesImpl());
    }

    class UInt32ArrayPrivate;

    class QSUBSTATE_EXPORT UInt32Array : public IntArrayBase {
        Q_OBJECT
        Q_DECLARE_PRIVATE(UInt32Array)
    public:
        UInt32Array(QObject *parent = nullptr);
        UInt32Array(Node *node, QObject *parent = nullptr);
        ~UInt32Array();

    public:
        inline int size() const;
        inline const quint32 *data() const;
        QVector<quint32> mid(int index, int size) const;
        QVector<quint32> values() const;
        void replace(int index, const QVector<quint32> &values);
        void insert(int index, const QVector<quint32> &values);
        void remove(int index, int count);

    Q_SIGNALS:
        void replaced(int index, const QVector<quint32> &values);
        void inserted(int index, const QVector<quint32> &values);
        void removed(int index, const QVector<quint32> &values);
    };

    inline int UInt32Array::size() const {
        return sizeImpl();
    }

    inline const quint32 *UInt32Array::data() const {
        return reinterpret_cast<const quint32 *>(valuesImpl());
    }

    class Int64ArrayPrivate;

    class QSUBSTATE_EXPORT Int64Array : public IntArrayBase {
        Q_OBJECT
        Q_DECLARE_PRIVATE(Int64Array)
    public:
        Int64Array(QObject *parent = nullptr);
        Int64Array(Node *node, QObject *parent = nullptr);
        ~Int64Array();

    public:
        inline int size() const;
        inline const qint64 *data() const;
        QVector<qint64> mid(int index, int size) const;
        QVector<qint64> values() const;
        void replace(int index, const QVector<qint64> &values);
        void insert(int index, const QVector<qint64> &values);
        void remove(int index, int count);

    Q_SIGNALS:
        void replaced(int index, const QVector<qint64> &values);
        void inserted(int index, const QVector<qint64> &values);
        void removed(int index, const QVector<qint64> &values);
    };

    inline int Int64Array::size() const {
        return sizeImpl();
    }

    inline const qint64 *Int64Array::data() const {
        return reinterpret_cast<const qint64 *>(valuesImpl());
    }

    class UInt64ArrayPrivate;

    class QSUBSTATE_EXPORT UInt64Array : public IntArrayBase {
        Q_OBJECT
        Q_DECLARE_PRIVATE(UInt64Array)
    public:
        UInt64Array(QObject *parent = nullptr);
        UInt64Array(Node *node, QObject *parent = nullptr);
        ~UInt64Array();

    public:
        inline int size() const;
        inline const quint64 *data() const;
        QVector<quint64> mid(int index, int size) const;
        QVector<quint64> values() const;
        void replace(int index, const QVector<quint64> &values);
        void insert(int index, const QVector<quint64> &values);
        void remove(int index, int count);

    Q_SIGNALS:
        void replaced(int index, const QVector<quint64> &values);
        void inserted(int index, const QVector<quint64> &values);
        void removed(int index, const QVector<quint64> &values);
    };

    inline int UInt64Array::size() const {
        return sizeImpl();
    }

    inline const quint64 *UInt64Array::data() const {
        return reinterpret_cast<const quint64 *>(valuesImpl());
    }

    class DoubleArrayPrivate;

    class QSUBSTATE_EXPORT DoubleArray : public IntArrayBase {
        Q_OBJECT
        Q_DECLARE_PRIVATE(DoubleArray)
    public:
        DoubleArray(QObject *parent = nullptr);
        DoubleArray(Node *node, QObject *parent = nullptr);
        ~DoubleArray();

    public:
        inline int size() const;
        inline const double *data() const;
        QVector<double> mid(int index, int size) const;
        QVector<double> values() const;
        void replace(int index, const QVector<double> &values);
        void insert(int index, const QVector<double> &values);
        void remove(int index, int count);

    Q_SIGNALS:
        void replaced(int index, const QVector<double> &values);
        void inserted(int index, const QVector<double> &values);
        void removed(int index, const QVector<double> &values);
    };

    inline int DoubleArray::size() const {
        return sizeImpl();
    }

    inline const double *DoubleArray::data() const {
        return reinterpret_cast<const double *>(valuesImpl());
    }

}

#endif // INTARRAY_H
