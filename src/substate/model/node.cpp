#include "node.h"
#include "node_p.h"

#include <mutex>
#include <shared_mutex>

#include "bytesnode_p.h"
#include "mappingnode_p.h"
#include "sheetnode_p.h"
#include "vectornode_p.h"

#include "model_p.h"

namespace Substate {

    static std::shared_mutex factoryLock;

    static std::unordered_map<int, Node::Factory> factoryManager;

    static inline Node::Factory getFactory(int type) {
        std::shared_lock<std::shared_mutex> lock(factoryLock);
        auto it = factoryManager.find(type);
        if (it == factoryManager.end()) {
            throw std::runtime_error("Substate::Node: Unknown node type " + std::to_string(type));
        }
        return it->second;
    }

    NodePrivate::NodePrivate(int type)
        : type(type), parent(nullptr), model(nullptr), index(0), managed(false),
          allowDelete(false) {
    }

    NodePrivate::~NodePrivate() {
        Q_Q(Node);

        if (model) {
            // The node is deleted by a wrong behavior in user code, the application must
            // abort otherwise the user data may be corrupted.
            if (!allowDelete) {
                SUBSTATE_FATAL("Deleting a managed item, crash now!!!");
            }

            // The node is deleted forcefully, which is possibly due to the following reasons.
            // 1. The node is no longer reachable
            // 2. The node is deleted from memory temporarily

            // We need to remove its reference in the index map except when the model is in
            // destruction so that there's no need.
            if (!model->isBeingDestroyed()) {
                model->d_func()->removeIndex(index);
            }
        } else if (parent && !parent->isBeingDestroyed()) {
            // The node is deleted when itself or its ancestor is free, simply release the
            // reference.
            parent->childDestroyed(q);
        }
    }

    void NodePrivate::init() {
    }

    void NodePrivate::setManaged(bool managed) {
        Q_Q(Node);
        q->propagate([&managed](Node *node) {
            node->d_func()->managed = managed; // Change managed flag recursively
        });
    }

    void NodePrivate::propagateModel(Substate::Model *model) {
        Q_Q(Node);
        auto model_d = model->d_func();
        q->propagate([q, &model, &model_d](Node *node) {
            auto d = node->d_func();
            d->index = model_d->addIndex(node, d->index);
            d->model = model;
        });
    }

    Node::Node(int type) : Node(*new NodePrivate(type)) {
    }

    Node::~Node() {
    }

    Node *Node::parent() const {
        Q_D(const Node);
        return d->parent;
    }

    Model *Node::model() const {
        Q_D(const Node);
        return d->model;
    }

    int Node::index() const {
        return 0;
    }

    Node::Type Node::type() const {
        Q_D(const Node);
        return d->type >= User ? User : static_cast<Type>(d->type);
    }

    int Node::userType() const {
        Q_D(const Node);
        return d->type;
    }

    bool Node::isFree() const {
        Q_D(const Node);
        return !d->model && !d->parent; // The item is not free if it has parent or model
    }

    bool Node::isManaged() const {
        Q_D(const Node);
        return d->managed;
    }

    bool Node::isWritable() const {
        Q_D(const Node);
        return !d->model || d->model->isWritable();
    }

    Node *Node::clone() const {
        return clone(true);
    }

    Node *Node::read(IStream &stream) {
        int type;
        stream >> type;
        if (stream.fail())
            return nullptr;

        switch (type) {
            case Bytes:
                return BytesNodePrivate::read(stream);
            case Vector:
                return VectorNodePrivate::read(stream);
            case Mapping:
                return MappingNodePrivate::read(stream);
            case Sheet:
                return SheetNodePrivate::read(stream);
            default:
                break;
        }

        return getFactory(type)(stream);
    }

    bool Node::registerFactory(int type, Factory fac) {
        std::unique_lock<std::shared_mutex> lock(factoryLock);

        auto it = factoryManager.find(type);
        if (it == factoryManager.end())
            return false;

        factoryManager.insert(std::make_pair(type, fac));
        return true;
    }

    void Node::childDestroyed(Node *node) {
    }

    void Node::propagateChildren(const std::function<void(Node *)> &func) {
    }

    void Node::addChild(Node *node) {
        Q_D(Node);
        auto d2 = node->d_func();
        d2->parent = d->parent;
        if (d2->managed) {
            d2->setManaged(false);
        }

        if (d->model && !d2->model) {
            d2->propagateModel(d->model);
        }
    }

    void Node::removeChild(Node *node) {
        Q_D(Node);
        auto d2 = node->d_func();
        d2->parent = nullptr;
        if (d->model) {
            d2->setManaged(true);
        }
    }

    void Node::beginAction() {
        Q_D(Node);
        d->model->d_func()->lockedNode = this;
    }

    void Node::endAction() {
        Q_D(Node);
        d->model->d_func()->lockedNode = nullptr;
    }

    void Node::dispatch(Action *action, bool done) {
        Q_D(Node);
        Sender::dispatch(action, done);

        if (!d->model)
            return;
        d->model->dispatch(action, done);
    }

    void Node::propagate(const std::function<void(Node *)> &func) {
        func(this);
        propagateChildren(func);
    }

    /*!
        \internal
    */
    Node::Node(NodePrivate &d) : Sender(d) {
        d.init();
    }

}
