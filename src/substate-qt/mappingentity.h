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
        struct Value {
            bool is_variant;
            union {
                Entity *item;
                const QVariant *variant;
            };
        };

        Value valueImpl(const QByteArray &key) const;
        bool setValueImpl(const QByteArray &key, const Value &value);
        QList<QByteArray> keysImpl() const;
        int sizeImpl() const;

        virtual void sendInserted(const QByteArray &key, const Value &val, const Value &oldVal) = 0;

    protected:
        MappingEntityBase(Node *node, QObject *parent = nullptr);
    };

}

#endif // MAPPINGENTITY_H
