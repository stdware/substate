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

        inline int type() const;

    public:
        virtual void execute(bool undo) = 0;

    protected:
        int t;
    };

    inline Operation::Operation(int type) : t(type) {
    }

    int Operation::type() const {
        return t;
    }

}

#endif // OPERATION_H
