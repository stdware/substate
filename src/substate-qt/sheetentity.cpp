#include "sheetentity.h"

#include <substate/sheetnode.h>

#include "entity_p.h"

namespace Substate {

    class SheetEntityBasePrivate : public EntityPrivate {
        Q_DECLARE_PUBLIC(SheetEntityBase)
    public:
        SheetEntityBasePrivate(Node *node) : EntityPrivate(node) {
        }

        ~SheetEntityBasePrivate() = default;

        void init() {
        }

        void notified(Notification *n) {
            switch (n->type()) {
                case Notification::ActionAboutToTrigger: {
                    auto n2 = static_cast<ActionNotification *>(n);
                    auto a = n2->action();
                    switch (a->type()) {
                        case Action::SheetRemove: {
                            auto a2 = static_cast<SheetAction *>(a);
                            sendAboutToRemove(a2->id(), a2->child());
                            break;
                        }
                        default:
                            break;
                    }
                    break;
                }

                case Notification::ActionTriggered: {
                    auto n2 = static_cast<ActionNotification *>(n);
                    auto a = n2->action();
                    switch (a->type()) {
                        case Action::SheetInsert: {
                            auto a2 = static_cast<SheetAction *>(a);
                            sendInserted(a2->id(), a2->child());
                            break;
                        }
                        case Action::SheetRemove: {
                            auto a2 = static_cast<SheetAction *>(a);
                            sendRemoved(a2->id(), a2->child());
                            break;
                        }
                        default:
                            break;
                    }
                    break;
                }

                default:
                    break;
            }
        }

        inline void sendInserted(int id, Node *node) {
            Q_Q(SheetEntityBase);

            Entity *item;
            if (external) {
                item = Entity::extractEntity(node);
            } else {
                item = Entity::createEntity(node);
            }

            q->sendInserted(id, item);
        }

        inline void sendAboutToRemove(int id, Node *node) {
            Q_Q(SheetEntityBase);
            q->sendAboutToRemove(id, Entity::extractEntity(node));
        }

        inline void sendRemoved(int id, Node *node) {
            Q_Q(SheetEntityBase);

            Q_UNUSED(node)
            q->sendRemoved(id, Entity::extractEntity(node));
        }

        bool external = false;
    };

    SheetEntityBase::~SheetEntityBase() {
    }

    int SheetEntityBase::insertImpl(Entity *item) {
        Q_D(SheetEntityBase);

        d->external = true;
        auto res = static_cast<SheetNode *>(d->internalData())->insert(item->internalData());
        d->external = false;
        return res;
    }

    bool SheetEntityBase::removeImpl(int id) {
        Q_D(SheetEntityBase);

        d->external = true;
        auto res = static_cast<SheetNode *>(d->internalData())->remove(id);
        d->external = false;
        return res;
    }

    bool SheetEntityBase::removeImpl(Entity *item) {
        Q_D(SheetEntityBase);

        d->external = true;
        auto res = static_cast<SheetNode *>(d->internalData())->remove(item->internalData());
        d->external = false;
        return res;
    }

    Entity *SheetEntityBase::valueImpl(int id) const {
        Q_D(const SheetEntityBase);
        return Entity::extractEntity(static_cast<SheetNode *>(d->internalData())->record(id));
    }

    int SheetEntityBase::indexOfImpl(Entity *item) const {
        Q_D(const SheetEntityBase);
        return static_cast<SheetNode *>(d->internalData())->indexOf(item->internalData());
    }

    int SheetEntityBase::sizeImpl() const {
        Q_D(const SheetEntityBase);
        return static_cast<SheetNode *>(d->internalData())->size();
    }

    SheetEntityBase::SheetEntityBase(Node *node, QObject *parent)
        : Entity(*new SheetEntityBasePrivate(node), parent) {
    }

    SheetEntityBase::SheetEntityBase(SheetEntityBasePrivate &d, QObject *parent)
        : Entity(d, parent) {
        d.init();
    }

}