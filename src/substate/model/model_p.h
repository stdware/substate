#ifndef MODEL_P_H
#define MODEL_P_H

#include <substate/model.h>
#include <substate/private/sender_p.h>

namespace Substate {

    class SUBSTATE_EXPORT ModelPrivate : public SenderPrivate {
        QMSETUP_DECL_PUBLIC(Model)
    public:
        ModelPrivate(Engine *engine);
        virtual ~ModelPrivate();
        void init();

        Engine *engine;
        Node *lockedNode;

        Model::State state;
        std::vector<Action *> txActions;

        Node *root;

        void setRootItem_helper(Node *node);
    };

}

#endif // MODEL_P_H
