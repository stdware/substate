#ifndef MODEL_H
#define MODEL_H

#include <substate/node.h>

namespace Substate {

    class ModelPrivate;

    class SUBSTATE_EXPORT Model {
    public:
        Model();
        virtual ~Model();

    protected:
        std::unique_ptr<ModelPrivate> d_ptr;

        Model(ModelPrivate &d);
    };

}

#endif // MODEL_H
