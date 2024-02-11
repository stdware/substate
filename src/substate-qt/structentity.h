#ifndef STRUCTENTITY_H
#define STRUCTENTITY_H

#include <qsubstate/entity.h>

namespace Substate {

    class StructEntityBasePrivate;

    class QSUBSTATE_EXPORT StructEntityBase : public Entity {
        Q_OBJECT
        Q_DECLARE_PRIVATE(StructEntityBase)
    public:
        ~StructEntityBase();

    protected:
        struct Value {
            bool is_variant;
            union {
                Entity *item;
                const QVariant *variant;
            };
        };

        Value valueImpl(int i) const;
        bool setValueImpl(int i, const Value &value);
        int sizeImpl() const;

        virtual void sendAssigned(int index, const Value &val, const Value &oldVal) = 0;

    protected:
        StructEntityBase(Node *node, QObject *parent = nullptr);
    };

}

#endif // STRUCTENTITY_H
