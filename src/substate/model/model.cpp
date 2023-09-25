#include "model.h"
#include "model_p.h"

namespace Substate {

    ModelPrivate::ModelPrivate() {
    }

    ModelPrivate::~ModelPrivate() {
    }

    void ModelPrivate::init() {
    }

    Model::Model() : Model(*new ModelPrivate()) {
    }

    Model::~Model() {
    }

    Model::Model(ModelPrivate &d) : d_ptr(&d) {
        d.q_ptr = this;
        d.init();
    }

}