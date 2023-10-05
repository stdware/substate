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

        using value_type = qint8;

    public:
        inline int size() const;
        QVector<value_type> mid(int index, int size) const;
        QVector<value_type> values() const;
        void replace(int index, const QVector<qint8> &values);
        void insert(int index, const QVector<qint8> &values);
        void remove(int index, int count);

    Q_SIGNALS:
        void replaced(int index, const QVector<value_type> &values);
        void inserted(int index, const QVector<value_type> &values);
        void removed(int index, const QVector<value_type> &values);
    };

    inline int Int8Array::size() const {
        return sizeImpl();
    }

    class UInt8ArrayPrivate;

    class QSUBSTATE_EXPORT UInt8Array : public IntArrayBase {
        Q_OBJECT
        Q_DECLARE_PRIVATE(UInt8Array)
    public:
        UInt8Array(QObject *parent = nullptr);
        UInt8Array(Node *node, QObject *parent = nullptr);
        ~UInt8Array();

        using value_type = quint8;

    public:
        inline int size() const;
        QVector<value_type> mid(int index, int size) const;
        QVector<value_type> values() const;
        void replace(int index, const QVector<value_type> &values);
        void insert(int index, const QVector<value_type> &values);
        void remove(int index, int count);

    Q_SIGNALS:
        void replaced(int index, const QVector<value_type> &values);
        void inserted(int index, const QVector<value_type> &values);
        void removed(int index, const QVector<value_type> &values);
    };

    inline int UInt8Array::size() const {
        return sizeImpl();
    }

    class Int16ArrayPrivate;

    class QSUBSTATE_EXPORT Int16Array : public IntArrayBase {
        Q_OBJECT
        Q_DECLARE_PRIVATE(Int16Array)
    public:
        Int16Array(QObject *parent = nullptr);
        Int16Array(Node *node, QObject *parent = nullptr);
        ~Int16Array();

        using value_type = qint16;

    public:
        inline int size() const;
        QVector<value_type> mid(int index, int size) const;
        QVector<value_type> values() const;
        void replace(int index, const QVector<value_type> &values);
        void insert(int index, const QVector<value_type> &values);
        void remove(int index, int count);

    Q_SIGNALS:
        void replaced(int index, const QVector<value_type> &values);
        void inserted(int index, const QVector<value_type> &values);
        void removed(int index, const QVector<value_type> &values);
    };

    inline int Int16Array::size() const {
        return sizeImpl();
    }

    class UInt16ArrayPrivate;

    class QSUBSTATE_EXPORT UInt16Array : public IntArrayBase {
        Q_OBJECT
        Q_DECLARE_PRIVATE(UInt16Array)
    public:
        UInt16Array(QObject *parent = nullptr);
        UInt16Array(Node *node, QObject *parent = nullptr);
        ~UInt16Array();

        using value_type = quint16;

    public:
        inline int size() const;
        QVector<value_type> mid(int index, int size) const;
        QVector<value_type> values() const;
        void replace(int index, const QVector<value_type> &values);
        void insert(int index, const QVector<value_type> &values);
        void remove(int index, int count);

    Q_SIGNALS:
        void replaced(int index, const QVector<value_type> &values);
        void inserted(int index, const QVector<value_type> &values);
        void removed(int index, const QVector<value_type> &values);
    };

    inline int UInt16Array::size() const {
        return sizeImpl();
    }

    class Int32ArrayPrivate;

    class QSUBSTATE_EXPORT Int32Array : public IntArrayBase {
        Q_OBJECT
        Q_DECLARE_PRIVATE(Int32Array)
    public:
        Int32Array(QObject *parent = nullptr);
        Int32Array(Node *node, QObject *parent = nullptr);
        ~Int32Array();

        using value_type = qint32;

    public:
        inline int size() const;
        QVector<value_type> mid(int index, int size) const;
        QVector<value_type> values() const;
        void replace(int index, const QVector<value_type> &values);
        void insert(int index, const QVector<value_type> &values);
        void remove(int index, int count);

    Q_SIGNALS:
        void replaced(int index, const QVector<value_type> &values);
        void inserted(int index, const QVector<value_type> &values);
        void removed(int index, const QVector<value_type> &values);
    };

    inline int Int32Array::size() const {
        return sizeImpl();
    }

    class UInt32ArrayPrivate;

    class QSUBSTATE_EXPORT UInt32Array : public IntArrayBase {
        Q_OBJECT
        Q_DECLARE_PRIVATE(UInt32Array)
    public:
        UInt32Array(QObject *parent = nullptr);
        UInt32Array(Node *node, QObject *parent = nullptr);
        ~UInt32Array();

        using value_type = quint32;

    public:
        inline int size() const;
        QVector<value_type> mid(int index, int size) const;
        QVector<value_type> values() const;
        void replace(int index, const QVector<value_type> &values);
        void insert(int index, const QVector<value_type> &values);
        void remove(int index, int count);

    Q_SIGNALS:
        void replaced(int index, const QVector<value_type> &values);
        void inserted(int index, const QVector<value_type> &values);
        void removed(int index, const QVector<value_type> &values);
    };

    inline int UInt32Array::size() const {
        return sizeImpl();
    }

    class Int64ArrayPrivate;

    class QSUBSTATE_EXPORT Int64Array : public IntArrayBase {
        Q_OBJECT
        Q_DECLARE_PRIVATE(Int64Array)
    public:
        Int64Array(QObject *parent = nullptr);
        Int64Array(Node *node, QObject *parent = nullptr);
        ~Int64Array();

        using value_type = qint64;

    public:
        inline int size() const;
        QVector<value_type> mid(int index, int size) const;
        QVector<value_type> values() const;
        void replace(int index, const QVector<value_type> &values);
        void insert(int index, const QVector<value_type> &values);
        void remove(int index, int count);

    Q_SIGNALS:
        void replaced(int index, const QVector<value_type> &values);
        void inserted(int index, const QVector<value_type> &values);
        void removed(int index, const QVector<value_type> &values);
    };

    inline int Int64Array::size() const {
        return sizeImpl();
    }

    class UInt64ArrayPrivate;

    class QSUBSTATE_EXPORT UInt64Array : public IntArrayBase {
        Q_OBJECT
        Q_DECLARE_PRIVATE(UInt64Array)
    public:
        UInt64Array(QObject *parent = nullptr);
        UInt64Array(Node *node, QObject *parent = nullptr);
        ~UInt64Array();

        using value_type = quint64;

    public:
        inline int size() const;
        QVector<value_type> mid(int index, int size) const;
        QVector<value_type> values() const;
        void replace(int index, const QVector<value_type> &values);
        void insert(int index, const QVector<value_type> &values);
        void remove(int index, int count);

    Q_SIGNALS:
        void replaced(int index, const QVector<value_type> &values);
        void inserted(int index, const QVector<value_type> &values);
        void removed(int index, const QVector<value_type> &values);
    };

    inline int UInt64Array::size() const {
        return sizeImpl();
    }

}

#endif // INTARRAY_H
