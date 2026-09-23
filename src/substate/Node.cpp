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

    void NodePrivate::pushAction(Model *model, std::unique_ptr<Action> action) {
        assert(model->inTransaction());
        model->m_actions.push_back(std::move(action));
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

    bool Node::isWritable() const {
        if (!m_model) {
            return true;
        }
        return m_model->inTransaction() && m_attached;
    }

}
