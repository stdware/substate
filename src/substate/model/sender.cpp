#include "sender.h"
#include "sender_p.h"

namespace Substate {

    SenderPrivate::SenderPrivate() {
        is_clearing = false;
    }

    SenderPrivate::~SenderPrivate() {
        is_clearing = true;

        for (const auto &sub : std::as_const(subscribers))
            sub->m_sender = nullptr;
    }

    void SenderPrivate::init() {
    }

    /*!
        \class Sender

        Sender is used to broadcast action messages to subscribers.
    */

    /*!
        Destructor.
    */
    Sender::~Sender() {
    }

    /*!
        Adds a subscriber.
    */
    void Sender::addSubscriber(Subscriber *sub) {
        Q_D(Sender);
        auto it = d->subscriberIndexes.find(sub);
        if (it != d->subscriberIndexes.end())
            return;
        d->subscriberIndexes.insert(
            std::make_pair(sub, d->subscribers.insert(d->subscribers.end(), sub)));

        sub->m_sender = this;
    }

    /*!
        Removes a subscriber.
    */
    void Sender::removeSubscriber(Subscriber *sub) {
        Q_D(Sender);
        auto it = d->subscriberIndexes.find(sub);
        if (it == d->subscriberIndexes.end())
            return;
        d->subscribers.erase(it->second);
        d->subscriberIndexes.erase(it);

        sub->m_sender = nullptr;
    }


    /*!
        Returns true if the sender is in destruction.
    */
    bool Sender::isBeingDestroyed() const {
        Q_D(const Sender);
        return d->is_clearing;
    }

    /*!
        Notifies all subscribers of the action message.
    */
    void Sender::dispatch(Action *action, bool done) {
        Q_D(Sender);

        for (const auto &sub : std::as_const(d->subscribers)) {
            sub->action(action, done);
        }
    }

    /*!
        \internal
    */
    Sender::Sender(SenderPrivate &d) : d_ptr(&d) {
        d.q_ptr = this;
        d.init();
    }

    /*!
        \class Subscriber

        Subscriber receives action messages from Sender.
    */

    /*!
        Constructor.
    */
    Subscriber::Subscriber() : m_sender(nullptr) {
    }

    /*!
        Destructor.
    */
    Subscriber::~Subscriber() {
        if (m_sender)
            m_sender->removeSubscriber(this);
    }

    /*!
        \fn Sender *Subscriber::sender() const

        Returns the sender.
    */

    /*!
        \fn void Subscriber::action(Action *action, bool done)

        Processes the current action massage.
    */

    Notification::Notification(int type) : t(type) {
    }

    Notification::~Notification() {
    }

}