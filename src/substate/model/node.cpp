#include "node.h"
#include "node_p.h"

#include <mutex>
#include <shared_mutex>

#include "bytesnode_p.h"
#include "mappingnode_p.h"
#include "sheetnode_p.h"
#include "vectornode_p.h"

#include "model_p.h"
#include "nodehelper.h"

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
        : type(type), parent(nullptr), model(nullptr), index(0), managed(false), allowDelete(false),
          extra(nullptr) {
    }

    NodePrivate::~NodePrivate() {
        QM_Q(Node);

        if (model) {
            // The node is deleted by a wrong behavior in user code, the application must
            // abort otherwise the user data may be corrupted.
            if (!allowDelete) {
                QMSETUP_FATAL("Deleting a managed item, crash now!!!");
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

        delete extra;
    }

    void NodePrivate::init() {
    }

    void NodePrivate::setManaged(bool _managed) {
        QM_Q(Node);
        q->propagate([&_managed](Node *node) {
            node->d_func()->managed = _managed; // Change managed flag recursively
        });
    }

    void NodePrivate::propagateModel(Substate::Model *_model) {
        QM_Q(Node);
        auto model_d = _model->d_func();
        q->propagate([&_model, &model_d](Node *node) {
            auto d = node->d_func();
            d->index = model_d->addIndex(node, d->index);
            d->model = _model;
        });
    }

    bool NodePrivate::testModifiable() const {
        return !managed && (!model || model->isWritable());
    }

    RootChangeAction *readRootChangeAction(IStream &stream,
                                           const std::unordered_map<int, Node *> &existingNodes) {
        int oldRootIndex, newRootIndex;
        stream >> oldRootIndex >> newRootIndex;

        Node *oldRoot = nullptr, *newRoot = nullptr;

        if (oldRootIndex != 0) {
            auto it = existingNodes.find(oldRootIndex);
            if (it == existingNodes.end())
                return nullptr;
            oldRoot = it->second;
        }

        if (newRootIndex != 0) {
            auto it = existingNodes.find(newRootIndex);
            if (it == existingNodes.end())
                return nullptr;
            newRoot = it->second;
        }

        return new RootChangeAction(newRoot, oldRoot);
    }

    Node::Node(int type) : Node(*new NodePrivate(type)) {
    }

    Node::~Node() {
    }

    int Node::type() const {
        QM_D(const Node);
        return d->type;
    }

    Node *Node::parent() const {
        QM_D(const Node);
        return d->parent;
    }

    Model *Node::model() const {
        QM_D(const Node);
        return d->model;
    }

    int Node::index() const {
        QM_D(const Node);
        return d->index;
    }

    bool Node::isFree() const {
        QM_D(const Node);
        return !d->model && !d->parent; // The item is not free if it has parent or model
    }

    bool Node::isManaged() const {
        QM_D(const Node);
        return d->managed;
    }

    bool Node::isWritable() const {
        QM_D(const Node);
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

    void Node::dispatch(Notification *n) {
        QM_D(Node);

        Sender::dispatch(n);

        if (d->extra) {
            d->extra->notified(n);
        }

        switch (n->type()) {
            case Notification::ActionAboutToTrigger:
            case Notification::ActionTriggered: {
                if (d->model) {
                    d->model->dispatch(n);
                }
                break;
            }
            default:
                break;
        }
    }

    void Node::childDestroyed(Node *node) {
    }

    void Node::propagateChildren(const std::function<void(Node *)> &func) {
    }

    void Node::addChild(Node *node) {
        QM_D(Node);
        auto d2 = node->d_func();
        d2->parent = d->parent;
        if (d2->managed) {
            d2->setManaged(false);
        }
    }

    void Node::removeChild(Node *node) {
        QM_D(Node);
        auto d2 = node->d_func();
        d2->parent = nullptr;
        if (d->model) {
            d2->setManaged(true);
        }
    }

    void Node::beginAction() {
        QM_D(Node);
        d->model->d_func()->lockedNode = this;
    }

    void Node::endAction() {
        QM_D(Node);
        d->model->d_func()->lockedNode = nullptr;
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

    NodeExtra::NodeExtra() : m_node(nullptr) {
    }

    NodeExtra::NodeExtra(Node *node) : m_node(node) {
        m_node->d_func()->extra = nullptr;
    }

    NodeExtra::~NodeExtra() {
        if (m_node)
            m_node->d_func()->extra = nullptr;
    }

    void NodeExtra::setInternalData(Node *node) {
        if (m_node) {
            m_node->d_func()->extra = nullptr;
        }
        m_node = node;
        if (m_node) {
            m_node->d_func()->extra = this;
        }
    }

    NodeAction::NodeAction(int type, Node *parent) : Action(type), m_parent(parent) {
    }

    NodeAction::~NodeAction() {
    }

    RootChangeAction::RootChangeAction(Node *root, Node *oldRoot)
        : Action(RootChange), r(root), oldr(oldRoot) {
    }

    RootChangeAction::~RootChangeAction() {
    }

    void RootChangeAction::write(OStream &stream) const {
        stream << (oldr ? oldr->index() : 0) << (r ? r->index() : 0);
    }

    Action *RootChangeAction::clone() const {
        return new RootChangeAction(r, oldr);
    }

    void RootChangeAction::execute(bool undo) {
        auto d = (r ? r->model() : oldr->model())->d_func();
        d->setRootItem_helper(undo ? oldr : r);
    }

    void RootChangeAction::virtual_hook(int id, void *data) {
        switch (id) {
            case CleanNodesHook: {
                NodeHelper::forceDelete(r);
                break;
            }
            case InsertedNodesHook: {
                if (r) {
                    auto &res = *reinterpret_cast<std::vector<Node *> *>(data);
                    res.push_back(r);
                }
                break;
            }
            case RemovedNodesHook: {
                if (oldr) {
                    auto &res = *reinterpret_cast<std::vector<Node *> *>(data);
                    res.push_back(oldr);
                }
                break;
            }
            default:
                break;
        }
    }

}
