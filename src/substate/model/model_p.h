#ifndef MODEL_P_H
#define MODEL_P_H

#include <substate/model.h>

namespace Substate {

    class SUBSTATE_EXPORT ModelPrivate {
        SUBSTATE_DECL_PUBLIC(Model)
    public:
        ModelPrivate();
        virtual ~ModelPrivate();

        void init();

        Model *q_ptr;
    };

}

#endif // MODEL_P_H
