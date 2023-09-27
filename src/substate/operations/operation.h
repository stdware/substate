#ifndef OPERATION_H
#define OPERATION_H

#include <memory>
#include <string>

#include <substate/stream.h>

namespace Substate {

    class SUBSTATE_EXPORT Operation {
    public:
        inline Operation(int type);
        virtual ~Operation();

        enum Type {
            BytesReplace,
            BytesInsert,
            BytesRemove,

            VectorInsert,
            VectorMove,
            VectorRemove,
            
            RecordInsert,
            RecordRemove,
            
            MappingInsert,
            MappingRemove,
            
            RootChange,

            User = 1024,
        };

        inline Type type() const;
        inline int userType() const;

        typedef Operation *(*Factory)(IStream &);

        static Operation *read(IStream &stream);
        static bool registerFactory(int type, Factory fac);

    public:
        virtual void execute(bool undo) = 0;

    protected:
        int t;
    };

    inline Operation::Operation(int type) : t(type) {
    }

    inline Operation::Type Operation::type() const {
        return t >= User ? User : static_cast<Type>(t);
    }

    inline int Operation::userType() const {
        return t;
    }

}

#endif // OPERATION_H
