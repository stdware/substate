#include "SheetNode.h"

#include <cassert>

#include "Node_p.h"

namespace ss {

    SheetNode::~SheetNode() = default;

    Node *SheetNode::at(int key) const {
        auto it = m_children.find(key);
        return it == m_children.end() ? nullptr : it->second.get();
    }

    std::vector<int> SheetNode::keys() const {
        std::vector<int> result;
        result.reserve(m_children.size());
        for (const auto &child : m_children) {
            result.push_back(child.first);
        }
        return result;
    }

    int SheetNode::insert(std::unique_ptr<Node> node) {
        assert(isWritable());
        assert(NodePrivate::isInsertable(node.get()));
        assert(!NodePrivate::isAncestorOrSelf(node.get(), this));

        const int key = ++m_lastKey;
        if (isFree()) {
            NodePrivate::setFreeParent(node.get(), this);
            m_children.emplace(key, std::move(node));
            return key;
        }

        std::unique_ptr<SheetInsDelAction> action(
            new SheetInsDelAction(Action::SheetInsert, this, key, std::move(node)));
        action->execute(Action::Execute);
        NodePrivate::pushAction(model(), std::move(action));
        return key;
    }

    bool SheetNode::remove(int key) {
        assert(isWritable());

        auto it = m_children.find(key);
        if (it == m_children.end()) {
            return false;
        }

        if (isFree()) {
            m_children.erase(it);
            return true;
        }

        std::unique_ptr<SheetInsDelAction> action(
            new SheetInsDelAction(Action::SheetRemove, this, key, nullptr));
        action->execute(Action::Execute);
        NodePrivate::pushAction(model(), std::move(action));
        return true;
    }

    std::unique_ptr<Node> SheetNode::take(int key) {
        assert(isFree());

        auto it = m_children.find(key);
        if (it == m_children.end()) {
            return nullptr;
        }
        auto node = std::move(it->second);
        m_children.erase(it);
        NodePrivate::setFreeParent(node.get(), nullptr);
        return node;
    }

    std::unique_ptr<Node> SheetNode::clone() const {
        std::unique_ptr<SheetNode> node(new SheetNode());
        node->cloneChildrenFrom(*this);
        return node;
    }

    void SheetNode::forEachChild(const std::function<void(Node *)> &func) const {
        for (const auto &child : m_children) {
            func(child.second.get());
        }
    }

    void SheetNode::cloneChildrenFrom(const SheetNode &source) {
        assert(isFree() && m_children.empty());
        for (const auto &child : source.m_children) {
            auto copy = child.second->clone();
            NodePrivate::setFreeParent(copy.get(), this);
            m_children.emplace(child.first, std::move(copy));
        }
        m_lastKey = source.m_lastKey;
    }

    SheetInsDelAction::SheetInsDelAction(int type, SheetNode *parent, int key,
                                         std::unique_ptr<Node> held)
        : Action(type), m_parent(parent), m_key(key),
          m_child(type == SheetInsert ? held.get() : parent->at(key)), m_held(std::move(held)) {
    }

    SheetInsDelAction::~SheetInsDelAction() = default;

    void SheetInsDelAction::forEachHeldNode(const std::function<void(Node *)> &func) const {
        if (m_held) {
            func(m_held.get());
        }
    }

    void SheetInsDelAction::execute(Operation operation) {
        auto &children = m_parent->m_children;
        const bool intoTree = (type() == SheetInsert) == isForward(operation);

        if (intoTree) {
            assert(m_held && children.find(m_key) == children.end());
            NodePrivate::attach(m_held.get(), m_parent, m_parent->model());
            children.emplace(m_key, std::move(m_held));
        } else {
            auto it = children.find(m_key);
            assert(!m_held && it != children.end() && it->second.get() == m_child);
            m_held = std::move(it->second);
            children.erase(it);
            NodePrivate::detach(m_held.get());        }
    }

}
