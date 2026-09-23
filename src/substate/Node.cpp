#include "Node.h"
#include "Node_p.h"

#include <cassert>

#include "Model.h"

namespace ss {

    void NodePrivate::forEachInSubtree(Node *node, const std::function<void(Node *)> &func) {
        func(node);
        node->forEachChild([&func](Node *child) { forEachInSubtree(child, func); });
    }

    void NodePrivate::attach(Node *node, Node *parent, Model *model) {
        assert(model);
        node->m_parent = parent;
        forEachInSubtree(node, [model](Node *n) {
            if (!n->m_model) {
                n->m_model = model;
                n->m_id = ++model->m_lastId;
                model->m_index.emplace(n->m_id, n);
            }
            assert(n->m_model == model);
            n->m_attached = true;
        });
    }

    void NodePrivate::detach(Node *node) {
        node->m_parent = nullptr;
        forEachInSubtree(node, [](Node *n) { n->m_attached = false; });
    }

    bool NodePrivate::transfer(Node *target, std::unique_ptr<TransferEndpoint> targetEnd,
                               const std::vector<Node *> &nodes) {
        assert(target->isWritable() && !target->isFree());
        assert(!nodes.empty());

        auto source = nodes.front()->parent();
        if (!source || source == target) {
            return false;
        }
        for (auto node : nodes) {
            assert(node->parent() == source && node->model() == target->model());
            assert(node->isAttached());
            if (isAncestorOrSelf(node, target)) {
                return false;
            }
        }

        auto sourceEnd = source->endpointOf(nodes);
        if (!sourceEnd) {
            return false;
        }

        execute(target->model(), std::unique_ptr<TransferAction>(new TransferAction(
                                     std::move(sourceEnd), std::move(targetEnd), nodes)));
        return true;
    }

    void NodePrivate::execute(Model *model, std::unique_ptr<Action> action) {
        assert(model->inTransaction());
        model->apply(*action, Action::Execute);
        model->m_actions.push_back(std::move(action));
    }

    void NodePrivate::aboutToDiscard(const Action &action) {
        action.forEachHeldNode([](Node *node) { node->m_model->aboutToDestroy(node); });
    }

    void NodePrivate::removeFromIndex(Node *node) {
        auto erased = node->m_model->m_index.erase(node->m_id);
        assert(erased == 1);
        (void) erased;
    }

    Node::~Node() {
        if (m_model) {
            NodePrivate::removeFromIndex(this);
        }
    }

    void Node::forEachChild(const std::function<void(Node *)> &func) const {
        (void) func;
    }

    std::unique_ptr<TransferEndpoint> Node::endpointOf(const std::vector<Node *> &children) {
        (void) children;
        return nullptr;
    }

    bool Node::isWritable() const {
        if (!m_model) {
            return true;
        }
        return m_model->inTransaction() && m_attached;
    }

}
