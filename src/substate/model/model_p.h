#ifndef MODEL_P_H
#define MODEL_P_H

#include <list>
#include <unordered_map>

#include <substate/model.h>

namespace Substate {

    class SUBSTATE_EXPORT ModelPrivate {
        SUBSTATE_DECL_PUBLIC(Model)
    public:
        ModelPrivate();
        virtual ~ModelPrivate();
        void init();
        Model *q_ptr;

        std::list<Subscriber *> subscribers;
        std::unordered_map<Subscriber *, decltype(subscribers)::iterator> subscriberIndexes;

        Engine *engine;
    };

}

#endif // MODEL_P_H
