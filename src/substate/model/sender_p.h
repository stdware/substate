#ifndef MESSAGE_P_H
#define MESSAGE_P_H

#include <list>
#include <shared_mutex>
#include <unordered_map>

#include <substate/sender.h>

namespace Substate {

    class SenderPrivate {
        QTMEDIATE_DECL_PUBLIC(Sender)
    public:
        SenderPrivate();
        virtual ~SenderPrivate();
        void init();
        Sender *q_ptr;

        std::list<Subscriber *> subscribers;
        std::unordered_map<Subscriber *, decltype(subscribers)::iterator> subscriberIndexes;
        bool is_clearing;

        std::shared_mutex shared_lock;
    };

}

#endif // MESSAGE_P_H
