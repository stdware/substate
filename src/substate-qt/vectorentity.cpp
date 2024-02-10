#include "vectorentity.h"

#include <substate/vectornode.h>

#include "entity_p.h"

namespace Substate {

    class VectorEntityBasePrivate : public EntityPrivate {
        Q_DECLARE_PUBLIC(VectorEntityBase)
    public:
        VectorEntityBasePrivate(Node *node) : EntityPrivate(node) {
        }

        ~VectorEntityBasePrivate() = default;

        void init() {
        }

        void notified(Notification *n) {
            switch (n->type()) {
                case Notification::ActionAboutToTrigger: {
                    auto n2 = static_cast<ActionNotification *>(n);
                    auto a = n2->action();
                    switch (a->type()) {
                        case Action::VectorMove: {
                            auto a2 = static_cast<VectorMoveAction *>(a);
                            sendAboutToMove(a2->index(), a2->index(), a2->count());
                            break;
                        }
                        case Action::VectorRemove: {
                            auto a2 = static_cast<VectorInsDelAction *>(a);
                            sendAboutToRemove(a2->index(), a2->children());
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
                        case Action::VectorInsert: {
                            auto a2 = static_cast<VectorInsDelAction *>(a);
                            sendInserted(a2->index(), a2->children());
                            break;
                        }
                        case Action::VectorMove: {
                            auto a2 = static_cast<VectorMoveAction *>(a);
                            sendMoved(a2->index(), a2->index(), a2->count());
                            break;
                        }
                        case Action::VectorRemove: {
                            auto a2 = static_cast<VectorInsDelAction *>(a);
                            sendRemoved(a2->index(), a2->children());
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

        inline void sendInserted(int index, const std::vector<Node *> &nodes) {
            Q_Q(VectorEntityBase);

            QVector<Entity *> items;
            items.reserve(int(nodes.size()));
            if (external) {
                for (const auto &node : nodes) {
                    items.append(Entity::extractEntity(node));
                }
            } else {
                for (const auto &node : nodes) {
                    items.append(Entity::createEntity(node));
                }
            }

            q->sendInserted(index, items);
        }

        inline void sendAboutToMove(int index, int count, int dest) {
            Q_Q(VectorEntityBase);
            q->sendAboutToMove(index, count, dest);
        }

        inline void sendMoved(int index, int count, int dest) {
            Q_Q(VectorEntityBase);
            q->sendMoved(index, count, dest);
        }

        inline void sendAboutToRemove(int index, const std::vector<Node *> &nodes) {
            Q_Q(VectorEntityBase);

            QVector<Entity *> items;
            items.reserve(int(nodes.size()));
            for (const auto &node : nodes) {
                items.append(Entity::extractEntity(node));
            }

            q->sendAboutToRemove(index, items);
        }

        inline void sendRemoved(int index, const std::vector<Node *> &nodes) {
            Q_Q(VectorEntityBase);
            q->sendRemoved(index, int(nodes.size()));

            for (const auto &node : nodes) {
                if (!node->isFree()) {
                    delete Entity::extractEntity(node);
                }
            }
        }

        bool external = false;
    };

    VectorEntityBase::~VectorEntityBase() {
    }

    bool VectorEntityBase::insertImpl(int index, const QVector<Entity *> &items) {
        Q_D(VectorEntityBase);

        std::vector<Node *> nodes;
        nodes.reserve(items.size());
        for (const auto &item : items) {
            nodes.push_back(item->internalData());
        }

        d->external = true;
        auto res = static_cast<VectorNode *>(d->internalData())->insert(index, nodes);
        d->external = false;
        return res;
    }

    bool VectorEntityBase::moveImpl(int index, int count, int dest) {
        Q_D(VectorEntityBase);

        d->external = true;
        auto res = static_cast<VectorNode *>(d->internalData())->move(index, count, dest);
        d->external = false;
        return res;
    }

    bool VectorEntityBase::removeImpl(int index, int count) {
        Q_D(VectorEntityBase);

        d->external = true;
        auto res = static_cast<VectorNode *>(d->internalData())->remove(index, count);
        d->external = false;
        return res;
    }

    Entity *VectorEntityBase::atImpl(int index) const {
        Q_D(const VectorEntityBase);
        return Entity::extractEntity(static_cast<VectorNode *>(d->internalData())->at(index));
    }

    int VectorEntityBase::sizeImpl() const {
        Q_D(const VectorEntityBase);
        return static_cast<VectorNode *>(d->internalData())->size();
    }

    VectorEntityBase::VectorEntityBase(Node *node, QObject *parent)
        : Entity(*new VectorEntityBasePrivate(node), parent) {
    }

    VectorEntityBase::VectorEntityBase(VectorEntityBasePrivate &d, QObject *parent)
        : Entity(d, parent) {
        d.init();
    }

}