#include "entity.h"
#include "entity_p.h"

#include <substate/nodehelper.h>

namespace Substate {

    EntityPrivate::EntityPrivate(Node *node) : NodeExtra(node) {
    }

    EntityPrivate::~EntityPrivate() {
    }

    void EntityPrivate::init() {
    }

    Entity::~Entity() {
    }

    Node *Entity::internalData(Node *node) const {
        Q_D(const Entity);
        return d->internalData();
    }

    Entity::Entity(EntityPrivate &d, QObject *parent) : QObject(parent), d_ptr(&d) {
        d.q_ptr = this;
        d.init();
    }

}