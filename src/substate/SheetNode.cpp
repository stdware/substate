#include "SheetNode.h"

#include <cassert>

#include "Node_p.h"

namespace ss {

    // The child under a key, as an end of a transfer. A key of 0 at the target is assigned by the
    // node at the first execution and kept for redo.
    class SheetNodeEndpoint : public TransferEndpoint {
    public:
        SheetNodeEndpoint(SheetNode *node, int key) : m_node(node), m_key(key) {
        }

        Node *container() const override {
            return m_node;
        }

        int key() const {
            return m_key;
        }

        std::vector<std::unique_ptr<Node>> take(int count) override {
            assert(count == 1);
            (void) count;
            auto it = m_node->m_children.find(m_key);
            assert(it != m_node->m_children.end());
            std::vector<std::unique_ptr<Node>> taken;
            taken.push_back(std::move(it->second));
            m_node->m_children.erase(it);
            return taken;
        }

        void put(std::vector<std::unique_ptr<Node>> nodes) override {
            assert(nodes.size() == 1);
            if (m_key == 0) {
                m_key = ++m_node->m_lastKey;
            }
            m_node->m_children.emplace(m_key, std::move(nodes.front()));
        }

    private:
        SheetNode *m_node;
        int m_key;
    };

    SheetNode::~SheetNode() = default;

    int SheetNode::transferIn(Node *node) {
        assert(isWritable() && !isFree());
        auto end = std::make_unique<SheetNodeEndpoint>(this, 0);
        auto raw = end.get();
        if (!NodePrivate::transfer(this, std::move(end), {node})) {
            return 0;
        }
        return raw->key();
    }

    std::unique_ptr<TransferEndpoint> SheetNode::endpointOf(const std::vector<Node *> &children) {
        if (children.size() != 1) {
            return nullptr;
        }
        for (const auto &child : m_children) {
            if (child.second.get() == children.front()) {
                return std::make_unique<SheetNodeEndpoint>(this, child.first);
            }
        }
        return nullptr;
    }

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
        NodePrivate::execute(model(), std::move(action));
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
        NodePrivate::execute(model(), std::move(action));
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
        if (isInsertion(operation)) {
            assert(m_held && children.find(m_key) == children.end());
            NodePrivate::attach(m_held.get(), m_parent, m_parent->model());
            children.emplace(m_key, std::move(m_held));
        } else {
            auto it = children.find(m_key);
            assert(!m_held && it != children.end() && it->second.get() == m_child);
            m_held = std::move(it->second);
            children.erase(it);
            NodePrivate::detach(m_held.get());
        }
    }

}
