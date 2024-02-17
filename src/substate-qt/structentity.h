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
        Value valueImpl(int i) const;
        bool setValueImpl(int i, const Value &value);
        inline bool setVariantImpl(int i, const QVariant &var);
        int sizeImpl() const;

        virtual void sendAssigned(int index, const Value &val, const Value &oldVal);

    protected:
        StructEntityBase(Node *node, QObject *parent = nullptr);
    };

    inline bool StructEntityBase::setVariantImpl(int i, const QVariant &var) {
        return setValueImpl(i, var);
    }

}

#endif // STRUCTENTITY_H
