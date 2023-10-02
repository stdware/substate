#include "entity.h"
#include "entity_p.h"

namespace Substate {

    EntityPrivate::EntityPrivate() {
    }

    EntityPrivate::~EntityPrivate() {
    }

    void EntityPrivate::init() {
    }

    Entity::~Entity() {
    }

    void Entity::notified(Substate::Notification *n) {
    }

    Entity::Entity(EntityPrivate &d, QObject *parent) : QObject(parent), d_ptr(&d) {
        d.q_ptr = this;
        d.init();
    }

}