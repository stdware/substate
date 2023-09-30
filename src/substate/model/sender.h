#ifndef SENDER_H
#define SENDER_H

#include <memory>

#include <substate/substate_global.h>

namespace Substate {

    class Notification;

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

    public:
        virtual void dispatch(Notification *n);

    protected:
        std::unique_ptr<SenderPrivate> d_ptr;
        Sender(SenderPrivate &d);

        SUBSTATE_DISABLE_COPY_MOVE(Sender)
    };

    class SUBSTATE_EXPORT Subscriber {
    public:
        Subscriber();
        virtual ~Subscriber();

    public:
        inline Sender *sender() const;

    protected:
        virtual void notified(Notification *n) = 0;

    private:
        Sender *m_sender;

        friend class Sender;
        friend class SenderPrivate;

        SUBSTATE_DISABLE_COPY_MOVE(Subscriber)
    };

    inline Sender *Subscriber::sender() const {
        return m_sender;
    }

    class SUBSTATE_EXPORT Notification {
    public:
        enum Type {
            ActionAboutToTrigger,
            ActionTriggered,
            StepChange,
            AboutToReset,
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
