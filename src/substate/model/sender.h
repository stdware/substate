#ifndef SENDER_H
#define SENDER_H

#include <memory>

#include <substate/substate_global.h>

namespace Substate {

    class Action;

    class Subscriber;

    class SenderPrivate;

    class SUBSTATE_EXPORT Sender {
        SUBSTATE_DECL_PRIVATE(Sender)
    public:
        virtual ~Sender();

    public:
        void addSubscriber(Subscriber *sub);
        void removeSubscriber(Subscriber *sub);

        bool isBeingDestroyed() const;

    protected:
        virtual void dispatch(Action *action, bool done);

    protected:
        std::unique_ptr<SenderPrivate> d_ptr;
        Sender(SenderPrivate &d);

        friend class Node;
        friend class NodePrivate;

        SUBSTATE_DISABLE_COPY_MOVE(Sender)
    };

    class SUBSTATE_EXPORT Subscriber {
    public:
        Subscriber();
        virtual ~Subscriber();

    public:
        inline Sender *sender() const;

    protected:
        virtual void action(Action *action, bool done) = 0;

    private:
        Sender *m_sender;

        friend class Sender;
        friend class SenderPrivate;

        SUBSTATE_DISABLE_COPY_MOVE(Subscriber)
    };

    inline Sender *Subscriber::sender() const {
        return m_sender;
    }

    class Notification {
    public:
        enum Type {
            Action,
            StepChange,
        };

        explicit Notification(int type);
        virtual ~Notification();

        inline int type() const;

    protected:
        int t;
    };

    inline int Notification::type() const {
        return t;
    }

}


#endif // SENDER_H
