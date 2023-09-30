#include "mappingnode.h"
#include "mappingnode_p.h"

#include <algorithm>
#include <cassert>

#include "nodehelper.h"

namespace Substate {

    MappingNodePrivate::MappingNodePrivate(int type) : NodePrivate(type) {
    }

    MappingNodePrivate::~MappingNodePrivate() {
        for (const auto &pair : std::as_const(mappingIndexes)) {
            delete pair.first;
        }
    }

    void MappingNodePrivate::init() {
    }

    MappingNode *MappingNodePrivate::read(IStream &stream) {
        auto node = new MappingNode();
        auto d = node->d_func();

        int size;
        stream >> d->index >> size;
        if (stream.fail()) {
            goto abort;
        }

        d->mappingIndexes.reserve(size);
        for (int i = 0; i < size; ++i) {
            std::string key;
            stream >> key;
            if (stream.fail())
                goto abort;
            auto child = Node::read(stream);
            if (!child) {
                goto abort;
            }

            node->addChild(child);
            d->mappingIndexes.insert(std::make_pair(child, key));
        }

        stream >> size;
        if (stream.fail())
            goto abort;

        d->mapping.reserve(d->mappingIndexes.size() + size);
        for (int i = 0; i < size; ++i) {
            std::string key;
            Variant var;
            stream >> key >> var;
            if (stream.fail())
                goto abort;
            d->mapping.insert(std::make_pair(key, var));
        }

        for (const auto &pair : std::as_const(d->mappingIndexes)) {
            d->mapping.insert(
                std::make_pair(pair.second, pair.first));
        }

        return node;

    abort:
        delete node;
        return nullptr;
    }

    void MappingNodePrivate::setProperty_helper(const std::string &key, const Data &data) {
        Q_Q(MappingNode);
        q->beginAction();

        Node *oldNode = nullptr;
        Variant oldValue;
        MappingNode::Value val, oldVal;

        if (data.variant.isValid()) {
            val = const_cast<Variant *>(&data.variant);
        } else {
            val = data.node;
        }

        auto it = mapping.find(key);
        if (it == mapping.end()) {
            // Nothing changes
            if (!val.isValid())
                return;
        } else {
            if (it->second.is_variant) {
                oldValue = it->second.variant;
                oldVal = &oldValue;
            } else {
                oldNode = it->second.node;
                oldVal = oldNode;
            }

            // Nothing changes
            if (val == oldVal)
                return;
        }

        MappingAction a(q, key, val, oldVal);

        // Pre-Propagate signal
        {
            ActionNotification n(Notification::ActionAboutToTrigger, &a);
            q->dispatch(&n);
        }

        // Do change
        if (it == mapping.end()) {
            mapping.insert(std::make_pair(key, data));
        } else {
            if (val.isValid()) {
                it->second = data;
            } else {
                mapping.erase(it);
            }
        }

        if (oldNode) {
            mappingIndexes.erase(oldNode);
            q->removeChild(oldNode);
        }
        if (data.node) {
            mappingIndexes.insert(std::make_pair(data.node, key));
            q->addChild(data.node);
        }

        // Propagate signal
        {
            ActionNotification n(Notification::ActionTriggered, &a);
            q->dispatch(&n);
        }

        q->endAction();
    }

    MappingNode::MappingNode() : Node(*new MappingNodePrivate(Mapping)) {
    }

    MappingNode::~MappingNode() {
    }

    MappingNode::Value MappingNode::property(const std::string &key) const {
        Q_D(const MappingNode);
        auto it = d->mapping.find(key);
        if (it == d->mapping.end()) {
            return {};
        }

        if (it->second.is_variant)
            return const_cast<Variant *>(&it->second.variant);
        return it->second.node;
    }

    void MappingNode::setProperty(const std::string &key, const Variant &value) {
        Q_D(MappingNode);
        assert(d->testModifiable());
        d->setProperty_helper(key, value);
    }

    void MappingNode::setProperty(const std::string &key, Node *node) {
        Q_D(MappingNode);
        assert(d->testModifiable());
        d->setProperty_helper(key, node);
    }

    std::vector<std::string> MappingNode::keys() const {
        Q_D(const MappingNode);
        std::vector<std::string> keys(d->mapping.size());
        std::transform(
            d->mapping.begin(), d->mapping.end(), keys.begin(),
            [](const std::pair<const std::string &, const MappingNodePrivate::Data &> &pair) {
                return pair.first;
            });
        return keys;
    }

    int MappingNode::size() const {
        Q_D(const MappingNode);
        return int(d->mapping.size());
    }

    void MappingNode::write(OStream &stream) const {
        Q_D(const MappingNode);
        // Write index
        stream << d->index;

        // Write nodes
        stream << int(d->mappingIndexes.size());
        for (const auto &pair : d->mappingIndexes) {
            stream << pair.second;     // key
            pair.first->write(stream); // node
        }

        // Write variants
        stream << int(d->mapping.size() - d->mappingIndexes.size());
        for (const auto &pair : d->mapping) {
            if (!pair.second.is_variant) {
                continue;
            }
            stream << pair.first;          // key
            stream << pair.second.variant; // variant
        }
    }

    Node *MappingNode::clone(bool user) const {
        Q_D(const MappingNode);

        auto node = new MappingNode();
        auto d2 = node->d_func();
        if (user)
            d2->index = d->index;

        // Clone children
        d2->mapping.reserve(d->mapping.size());
        for (auto &pair : d->mapping) {
            const auto &key = pair.first;
            const auto &data = pair.second;

            if (data.is_variant) {
                d2->mapping.insert(
                    std::make_pair(key, data.variant));
                continue;
            }

            auto newChild = NodeHelper::clone(data.node, user);
            node->addChild(newChild);

            d2->mapping.insert(std::make_pair(key, newChild));
            d2->mappingIndexes.insert(std::make_pair(newChild, key));
        }
        return node;
    }

    void MappingNode::childDestroyed(Node *node) {
        Q_D(MappingNode);
        auto it = d->mappingIndexes.find(node);
        if (it == d->mappingIndexes.end())
            return;
        d->mappingIndexes.erase(node);
        d->mapping.erase(it->second);
    }

    void MappingNode::propagateChildren(const std::function<void(Node *)> &func) {
        Q_D(MappingNode);
        for (const auto &pair : std::as_const(d->mappingIndexes)) {
            func(pair.first);
        }
    }

    MappingNode::MappingNode(MappingNodePrivate &d) : Node(d) {
        d.init();
    }

    MappingAction::MappingAction(Node *parent, const std::string &key,
                                 const MappingNode::Value &value,
                                 const MappingNode::Value &oldValue)
        : NodeAction(MappingSet, parent), m_key(key) {
        // Create copies
        v = value.isVariant() ? MappingNodePrivate::createValue(new Variant(value.variant()))
                              : MappingNodePrivate::createValue(value.node());
        oldv = value.isVariant() ? MappingNodePrivate::createValue(new Variant(oldValue.variant()))
                                 : MappingNodePrivate::createValue(oldValue.node());
    }

    MappingAction::~MappingAction() {
        // Destroy copies
        if (v.isVariant()) {
            delete reinterpret_cast<Variant *>(v.internalPointer());
        }
        if (oldv.isVariant()) {
            delete reinterpret_cast<Variant *>(oldv.internalPointer());
        }
    }

    void MappingAction::write(OStream &stream) const {
    }

    Action *MappingAction::clone() const {
        return new MappingAction(m_parent, m_key, v, oldv);
    }

    void MappingAction::execute(bool undo) {
    }

    void MappingAction::virtual_hook(int id, void *data) {
        switch (id) {
            case CleanNodesHook: {
                if (v.isNode()) {
                    NodeHelper::forceDelete(v.node());
                }
                break;
            }
            case InsertedNodesHook: {
                if (v.isNode()) {
                    auto &res = *reinterpret_cast<std::vector<Node *> *>(data);
                    res.push_back(v.node());
                }
                break;
            }
            case RemovedNodesHook: {
                if (oldv.isNode()) {
                    auto &res = *reinterpret_cast<std::vector<Node *> *>(data);
                    res.push_back(oldv.node());
                }
                break;
            }
            default:
                break;
        }
    }

}