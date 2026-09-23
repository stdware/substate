#include "MappingNode.h"

#include <cassert>

#include <substate/private/Node_p.h>

#include "Property_p.h"

namespace ss {

    // An entry, as an end of a transfer. The entry exists exactly while it holds the child.
    class MappingNodeEndpoint : public TransferEndpoint {
    public:
        MappingNodeEndpoint(MappingNode *node, QString key) : m_node(node), m_key(std::move(key)) {
        }

        Node *container() const override {
            return m_node;
        }

        std::vector<std::unique_ptr<Node>> take(int count) override {
            assert(count == 1);
            (void) count;
            auto it = m_node->m_entries.find(m_key);
            assert(it != m_node->m_entries.end());
            std::vector<std::unique_ptr<Node>> taken;
            taken.push_back(PropertyPrivate::releaseChild(it->second));
            m_node->m_entries.erase(it);
            return taken;
        }

        void put(std::vector<std::unique_ptr<Node>> nodes) override {
            assert(nodes.size() == 1 && !m_node->contains(m_key));
            m_node->m_entries.emplace(m_key, Property(std::move(nodes.front())));
        }

    private:
        MappingNode *m_node;
        QString m_key;
    };

    MappingNode::~MappingNode() = default;

    bool MappingNode::transferIn(const QString &key, Node *node) {
        assert(isWritable() && !isFree());
        if (contains(key)) {
            return false;
        }
        return NodePrivate::transfer(this, std::make_unique<MappingNodeEndpoint>(this, key),
                                     {node});
    }

    std::unique_ptr<TransferEndpoint> MappingNode::endpointOf(const std::vector<Node *> &children) {
        if (children.size() != 1) {
            return nullptr;
        }
        for (const auto &entry : m_entries) {
            if (entry.second.child() == children.front()) {
                return std::make_unique<MappingNodeEndpoint>(this, entry.first);
            }
        }
        return nullptr;
    }

    bool MappingNode::contains(const QString &key) const {
        return m_entries.find(key) != m_entries.end();
    }

    const Property &MappingNode::at(const QString &key) const {
        static const Property empty;
        auto it = m_entries.find(key);
        return it == m_entries.end() ? empty : it->second;
    }

    QStringList MappingNode::keys() const {
        QStringList result;
        result.reserve(qsizetype(m_entries.size()));
        for (const auto &entry : m_entries) {
            result.append(entry.first);
        }
        return result;
    }

    bool MappingNode::setProperty(const QString &key, Property value) {
        assert(isWritable());
        assert(PropertyPrivate::isAssignable(value, this));

        if (at(key) == value) {
            return false;
        }

        if (isFree()) {
            if (value.isEmpty()) {
                m_entries.erase(key);
            } else {
                PropertyPrivate::assignFree(this, m_entries[key], std::move(value));
            }
            return true;
        }

        std::unique_ptr<MappingAssignAction> action(
            new MappingAssignAction(this, key, std::move(value)));
        NodePrivate::execute(model(), std::move(action));
        return true;
    }

    Property MappingNode::take(const QString &key) {
        assert(isFree());
        auto it = m_entries.find(key);
        if (it == m_entries.end()) {
            return {};
        }
        Property value = PropertyPrivate::take(it->second);
        m_entries.erase(it);
        return value;
    }

    std::unique_ptr<Node> MappingNode::clone() const {
        std::unique_ptr<MappingNode> node(new MappingNode());
        node->cloneEntriesFrom(*this);
        return node;
    }

    void MappingNode::forEachChild(const std::function<void(Node *)> &func) const {
        for (const auto &entry : m_entries) {
            if (auto child = entry.second.child()) {
                func(child);
            }
        }
    }

    void MappingNode::cloneEntriesFrom(const MappingNode &source) {
        assert(isFree() && m_entries.empty());
        for (const auto &entry : source.m_entries) {
            PropertyPrivate::assignFree(this, m_entries[entry.first], entry.second.clone());
        }
    }

    MappingAssignAction::MappingAssignAction(MappingNode *parent, QString key, Property value)
        : PropertyAction(MappingAssign, parent, parent->at(key), std::move(value)),
          m_key(std::move(key)) {
    }

    MappingAssignAction::~MappingAssignAction() = default;

    void MappingAssignAction::execute(Operation operation) {
        (void) operation;
        auto &entries = static_cast<MappingNode *>(parent())->m_entries;
        auto it = entries.try_emplace(m_key).first;
        exchange(it->second);
        // An entry never holds an empty value.
        if (it->second.isEmpty()) {
            entries.erase(it);
        }
    }

}
