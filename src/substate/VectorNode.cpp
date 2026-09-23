#include "VectorNode.h"

#include <algorithm>
#include <cassert>
#include <iterator>

#include "Node_p.h"

namespace ss {

    namespace {

        using Children = std::vector<std::unique_ptr<Node>>;

        // Moves the range [index, index + count) so that it starts at destination afterwards.
        void moveRange(Children &children, int index, int count, int destination) {
            auto begin = children.begin();
            if (destination < index) {
                std::rotate(begin + destination, begin + index, begin + index + count);
            } else {
                std::rotate(begin + index, begin + index + count, begin + destination + count);
            }
        }

    }

    // A range of children starting at an index, as an end of a transfer.
    class VectorNodeEndpoint : public TransferEndpoint {
    public:
        VectorNodeEndpoint(VectorNode *node, int index) : m_node(node), m_index(index) {
        }

        Node *container() const override {
            return m_node;
        }

        std::vector<std::unique_ptr<Node>> take(int count) override {
            auto &children = m_node->m_children;
            auto first = children.begin() + m_index;
            auto last = first + count;
            std::vector<std::unique_ptr<Node>> taken(std::make_move_iterator(first),
                                                     std::make_move_iterator(last));
            children.erase(first, last);
            return taken;
        }

        void put(std::vector<std::unique_ptr<Node>> nodes) override {
            auto &children = m_node->m_children;
            children.insert(children.begin() + m_index, std::make_move_iterator(nodes.begin()),
                            std::make_move_iterator(nodes.end()));
        }

    private:
        VectorNode *m_node;
        int m_index;
    };

    VectorNode::~VectorNode() = default;

    bool VectorNode::transferIn(int index, const std::vector<Node *> &nodes) {
        assert(isWritable() && !isFree());
        assert(NodePrivate::isValidInsertion(index, size()));
        return NodePrivate::transfer(this, std::make_unique<VectorNodeEndpoint>(this, index),
                                     nodes);
    }

    std::unique_ptr<TransferEndpoint> VectorNode::endpointOf(const std::vector<Node *> &children) {
        auto first = std::find_if(m_children.begin(), m_children.end(), [&](const auto &child) {
            return child.get() == children.front();
        });
        if (first == m_children.end() ||
            m_children.end() - first < std::ptrdiff_t(children.size())) {
            return nullptr;
        }
        for (size_t i = 0; i < children.size(); ++i) {
            if (first[std::ptrdiff_t(i)].get() != children[i]) {
                return nullptr;
            }
        }
        return std::make_unique<VectorNodeEndpoint>(this, int(first - m_children.begin()));
    }

    void VectorNode::insert(int index, std::vector<std::unique_ptr<Node>> nodes) {
        assert(isWritable());
        assert(NodePrivate::isValidInsertion(index, size()));
        assert(!nodes.empty());
#ifndef NDEBUG
        for (const auto &node : nodes) {
            assert(NodePrivate::isInsertable(node.get()));
            assert(!NodePrivate::isAncestorOrSelf(node.get(), this));
        }
#endif

        if (isFree()) {
            for (const auto &node : nodes) {
                NodePrivate::setFreeParent(node.get(), this);
            }
            m_children.insert(m_children.begin() + index, std::make_move_iterator(nodes.begin()),
                              std::make_move_iterator(nodes.end()));
            return;
        }

        std::unique_ptr<VectorInsDelAction> action(new VectorInsDelAction(
            Action::VectorInsert, this, index, int(nodes.size()), std::move(nodes)));
        action->execute(Action::Execute);
        NodePrivate::pushAction(model(), std::move(action));
    }

    void VectorNode::remove(int index, int count) {
        assert(isWritable());
        assert(NodePrivate::isValidRemoval(index, count, size()));

        if (isFree()) {
            m_children.erase(m_children.begin() + index, m_children.begin() + index + count);
            return;
        }

        std::unique_ptr<VectorInsDelAction> action(
            new VectorInsDelAction(Action::VectorRemove, this, index, count, {}));
        action->execute(Action::Execute);
        NodePrivate::pushAction(model(), std::move(action));
    }

    void VectorNode::move(int index, int count, int destination) {
        assert(isWritable());
        assert(NodePrivate::isValidRemoval(index, count, size()));
        assert(destination >= 0 && destination <= size() - count && destination != index);

        if (isFree()) {
            moveRange(m_children, index, count, destination);
            return;
        }

        std::unique_ptr<VectorMoveAction> action(
            new VectorMoveAction(this, index, count, destination));
        action->execute(Action::Execute);
        NodePrivate::pushAction(model(), std::move(action));
    }

    std::vector<std::unique_ptr<Node>> VectorNode::take(int index, int count) {
        assert(isFree());
        assert(NodePrivate::isValidRemoval(index, count, size()));

        auto first = m_children.begin() + index;
        auto last = first + count;
        std::vector<std::unique_ptr<Node>> taken(std::make_move_iterator(first),
                                                 std::make_move_iterator(last));
        m_children.erase(first, last);
        for (const auto &node : taken) {
            NodePrivate::setFreeParent(node.get(), nullptr);
        }
        return taken;
    }

    std::unique_ptr<Node> VectorNode::clone() const {
        std::unique_ptr<VectorNode> node(new VectorNode());
        node->cloneChildrenFrom(*this);
        return node;
    }

    void VectorNode::forEachChild(const std::function<void(Node *)> &func) const {
        for (const auto &child : m_children) {
            func(child.get());
        }
    }

    void VectorNode::cloneChildrenFrom(const VectorNode &source) {
        assert(isFree() && m_children.empty());
        m_children.reserve(source.m_children.size());
        for (const auto &child : source.m_children) {
            auto copy = child->clone();
            NodePrivate::setFreeParent(copy.get(), this);
            m_children.push_back(std::move(copy));
        }
    }

    VectorInsDelAction::VectorInsDelAction(int type, VectorNode *parent, int index, int count,
                                           std::vector<std::unique_ptr<Node>> held)
        : Action(type), m_parent(parent), m_index(index), m_held(std::move(held)) {
        m_children.reserve(size_t(count));
        if (type == VectorInsert) {
            for (const auto &node : m_held) {
                m_children.push_back(node.get());
            }
        } else {
            for (int i = 0; i < count; ++i) {
                m_children.push_back(parent->at(index + i));
            }
        }
    }

    VectorInsDelAction::~VectorInsDelAction() = default;

    void VectorInsDelAction::forEachHeldNode(const std::function<void(Node *)> &func) const {
        for (const auto &node : m_held) {
            func(node.get());
        }
    }

    void VectorInsDelAction::execute(Operation operation) {
        auto &children = m_parent->m_children;
        const bool intoTree = (type() == VectorInsert) == isForward(operation);

        if (intoTree) {
            assert(m_held.size() == m_children.size());
            for (const auto &node : m_held) {
                NodePrivate::attach(node.get(), m_parent, m_parent->model());
            }
            children.insert(children.begin() + m_index, std::make_move_iterator(m_held.begin()),
                            std::make_move_iterator(m_held.end()));
            m_held.clear();
        } else {
            assert(m_held.empty());
            auto first = children.begin() + m_index;
            auto last = first + std::ptrdiff_t(m_children.size());
            m_held.assign(std::make_move_iterator(first), std::make_move_iterator(last));
            children.erase(first, last);
            for (const auto &node : m_held) {
                NodePrivate::detach(node.get());
            }
        }
    }

    VectorMoveAction::VectorMoveAction(VectorNode *parent, int index, int count, int destination)
        : Action(VectorMove), m_parent(parent), m_index(index), m_count(count),
          m_destination(destination) {
    }

    VectorMoveAction::~VectorMoveAction() = default;

    void VectorMoveAction::execute(Operation operation) {
        if (isForward(operation)) {
            moveRange(m_parent->m_children, m_index, m_count, m_destination);
        } else {
            moveRange(m_parent->m_children, m_destination, m_count, m_index);
        }
    }

}
