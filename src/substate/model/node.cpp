#include "node.h"
#include "node_p.h"

#include <mutex>
#include <shared_mutex>

#include "bytesnode_p.h"
#include "mappingnode_p.h"
#include "sheetnode_p.h"
#include "vectornode_p.h"

#include "model.h"

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

    NodePrivate::NodePrivate(int type) : type(type), parent(nullptr), model(nullptr) {
    }

    NodePrivate::~NodePrivate() {
    }

    void NodePrivate::init() {
    }

    Node::Node(int type) : Node(*new NodePrivate(type)) {
    }

    Node::~Node() {
        auto parent = this->parent();
        if (parent)
            parent->childDestroyed(this);
    }

    Node *Node::parent() const {
        Q_D(const Node);
        return d->parent;
    }

    Model *Node::model() const {
        Q_D(const Node);
        return d->model;
    }

    Node::Type Node::type() const {
        Q_D(const Node);
        return d->type >= User ? User : static_cast<Type>(d->type);
    }

    int Node::userType() const {
        Q_D(const Node);
        return d->type;
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

    void Node::addChild(Node *node) {
        Q_D(Node);

        // Assign parent
        node->d_func()->parent = d->parent;
    }

    void Node::removeChild(Node *node) {
        Q_D(Node);

        // Unassign parent
        node->d_func()->parent = d->parent;
    }

    void Node::dispatch(Operation *op, bool done) {
        Q_D(Node);
        if (!d->model)
            return;
        d->model->dispatch(op, done);
    }

    Node::Node(NodePrivate &d) : d_ptr(&d) {
        d.q_ptr = this;
        d.init();
    }

}
