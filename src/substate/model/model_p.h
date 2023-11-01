#ifndef MODEL_P_H
#define MODEL_P_H

#include <substate/model.h>
#include <substate/private/sender_p.h>

namespace Substate {

    class SUBSTATE_EXPORT ModelPrivate : public SenderPrivate {
        QTMEDIATE_DECL_PUBLIC(Model)
    public:
        ModelPrivate(Engine *engine);
        virtual ~ModelPrivate();
        void init();

        Engine *engine;
        Node *lockedNode;

        Model::State state;
        std::vector<Action *> txActions;
        std::unordered_map<int, Node *> indexes;
        int maxIndex;

        Node *root;

        int addIndex(Node *node, int idx = 0);
        void removeIndex(int index);

        void setRootItem_helper(Node *node);
    };

}

#endif // MODEL_P_H
