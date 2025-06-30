#ifndef SUBSTATE_MODEL_P_H
#define SUBSTATE_MODEL_P_H

#include <substate/Model.h>

namespace ss {

    class SUBSTATE_EXPORT ModelPrivate {
    public:
        static void setRoot_TX(Model *model, const std::shared_ptr<Node> &node);

        /// Sets the root node of the model silently, without creating any actions.
        static inline void setRoot(Model *model, const std::shared_ptr<Node> &node) {
            node->propagate([model](Node *n) { n->_model = model; });
            model->_root = node;
            node->_state = Node::Created;
        }
    };

}

#endif // SUBSTATE_MODEL_P_H