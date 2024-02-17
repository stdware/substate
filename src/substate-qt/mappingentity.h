#ifndef MAPPINGENTITY_H
#define MAPPINGENTITY_H

#include <QtCore/QByteArray>

#include <qsubstate/entity.h>

namespace Substate {

    class MappingEntityBasePrivate;

    class QSUBSTATE_EXPORT MappingEntityBase : public Entity {
        Q_OBJECT
        Q_DECLARE_PRIVATE(MappingEntityBase)
    public:
        ~MappingEntityBase();

    protected:
        Value valueImpl(const QByteArray &key) const;
        bool setValueImpl(const QByteArray &key, const Value &value);
        inline bool setVariantImpl(const QByteArray &key, const QVariant &var);
        QList<QByteArray> keysImpl() const;
        int sizeImpl() const;

        virtual void sendInserted(const QByteArray &key, const Value &val, const Value &oldVal);

    protected:
        MappingEntityBase(Node *node, QObject *parent = nullptr);
    };

    inline bool MappingEntityBase::setVariantImpl(const QByteArray &key, const QVariant &var) {
        return setValueImpl(key, var);
    }

}

#endif // MAPPINGENTITY_H
