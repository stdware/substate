#include "mappingnode.h"
#include "mappingnode_p.h"

#include <algorithm>
#include <cassert>

#include "nodehelper.h"

namespace Substate {

    MappingNodePrivate::MappingNodePrivate(const std::string &name)
        : NodePrivate(Node::Mapping, name) {
    }

    MappingNodePrivate::~MappingNodePrivate() {
        for (const auto &pair : std::as_const(mappingIndexes)) {
            delete pair.first;
        }
    }

    void MappingNodePrivate::init() {
    }

    MappingNode *MappingNodePrivate::read(IStream &stream) {
        std::string name;
        stream >> name;
        if (stream.fail())
            return nullptr;

        auto node = new MappingNode(name);
        auto d = node->d_func();

        stream >> d->index;
        if (stream.fail()) {
            goto abort;
        }

        stream >> d->mapping;
        if (stream.fail()) {
            goto abort;
        }

        for (const auto &item : std::as_const(d->mapping)) {
            const auto &key = item.first;
            const auto &prop = item.second;
            if (prop.isNode()) {
                node->addChild(prop.node());
                d->mappingIndexes.insert(std::make_pair(prop.node(), key));
            }
        }
        return node;

    abort:
        delete node;
        return nullptr;
    }

    void MappingNodePrivate::setProperty_helper(const std::string &key, const Property &prop) {
        QM_Q(MappingNode);
        q->beginAction();

        Property oldProp;

        auto it = mapping.find(key);
        if (it == mapping.end()) {
            // Nothing changes
            if (!prop.isValid())
                return;
        } else {
            // Nothing changes
            if (prop == it->second)
                return;

            oldProp = it->second;
        }

        MappingAction a(q, key, prop, oldProp);

        // Pre-Propagate signal
        {
            ActionNotification n(Notification::ActionAboutToTrigger, &a);
            q->dispatch(&n);
        }

        // Do change
        if (it == mapping.end()) {
            mapping.insert(std::make_pair(key, prop));
        } else {
            if (prop.isValid()) {
                it->second = prop;
            } else {
                mapping.erase(it);
            }
        }

        if (oldProp.isNode()) {
            auto oldNode = oldProp.node();
            mappingIndexes.erase(oldNode);
            q->removeChild(oldNode);
        }
        if (prop.node()) {
            auto node = prop.node();
            mappingIndexes.insert(std::make_pair(node, key));
            q->addChild(node);
        }

        // Propagate signal
        {
            ActionNotification n(Notification::ActionTriggered, &a);
            q->dispatch(&n);
        }

        q->endAction();
    }

    MappingAction *readMappingAction(IStream &stream) {
        int parentIndex;
        std::string key;

        stream >> parentIndex >> key;
        if (stream.fail())
            return nullptr;

        auto parent = reinterpret_cast<Node *>(uintptr_t(parentIndex));

        auto v = Property::read(stream);
        if (stream.fail())
            return nullptr;

        auto oldv = Property::read(stream);
        if (stream.fail())
            return nullptr;

        auto a = new MappingAction(parent, key, v, oldv);
        a->setState(Action::Unreferenced);
        return a;
    }

    MappingNode::MappingNode(const std::string &name) : Node(*new MappingNodePrivate(name)) {
    }

    MappingNode::~MappingNode() {
    }

    Property MappingNode::property(const std::string &key) const {
        QM_D(const MappingNode);
        auto it = d->mapping.find(key);
        if (it == d->mapping.end()) {
            return {};
        }
        return it->second;
    }

    bool MappingNode::setProperty(const std::string &key, const Property &value) {
        QM_D(MappingNode);
        assert(d->testModifiable());
        d->setProperty_helper(key, value);
        return true;
    }

    bool MappingNode::remove(Node *node) {
        QM_D(MappingNode);
        assert(d->testModifiable());

        // Validate
        if (!node) {
            QMSETUP_WARNING("trying to remove a null node from %p", this);
            return false;
        }

        auto it = d->mappingIndexes.find(node);
        if (it == d->mappingIndexes.end()) {
            QMSETUP_WARNING("node %p is not the child of %p", node, this);
            return false;
        }

        d->setProperty_helper(it->second, {});
        return true;
    }

    std::string MappingNode::indexOf(Node *node) const {
        QM_D(const MappingNode);
        auto it = d->mappingIndexes.find(node);
        if (it == d->mappingIndexes.end()) {
            return {};
        }
        return it->second;
    }

    std::vector<std::string> MappingNode::keys() const {
        QM_D(const MappingNode);
        std::vector<std::string> keys(d->mapping.size());
        std::transform(d->mapping.begin(), d->mapping.end(), keys.begin(),
                       [](const std::pair<const std::string &, const Property &> &pair) {
                           return pair.first;
                       });
        return keys;
    }

    const std::unordered_map<std::string, Property> &MappingNode::data() const {
        QM_D(const MappingNode);
        return d->mapping;
    }

    int MappingNode::size() const {
        QM_D(const MappingNode);
        return int(d->mapping.size());
    }

    void MappingNode::write(OStream &stream) const {
        QM_D(const MappingNode);
        stream << d->name << d->index << d->mapping;
    }

    Node *MappingNode::clone(bool user) const {
        QM_D(const MappingNode);

        auto node = new MappingNode(d->name);
        auto d2 = node->d_func();
        if (!user)
            d2->index = d->index;

        // Clone children
        d2->mapping.reserve(d->mapping.size());
        for (auto &pair : d->mapping) {
            const auto &key = pair.first;
            const auto &data = pair.second;

            if (data.isVariant()) {
                d2->mapping.insert(std::make_pair(key, data.variant()));
                continue;
            }

            auto newChild = NodeHelper::clone(data.node(), user);
            node->addChild(newChild);

            d2->mapping.insert(std::make_pair(key, newChild));
            d2->mappingIndexes.insert(std::make_pair(newChild, key));
        }
        return node;
    }

    void MappingNode::childDestroyed(Node *node) {
        remove(node);
    }

    void MappingNode::propagateChildren(const std::function<void(Node *)> &func) {
        QM_D(MappingNode);
        for (const auto &pair : std::as_const(d->mappingIndexes)) {
            NodeHelper::propagateNode(pair.first, func);
        }
    }

    MappingNode::MappingNode(MappingNodePrivate &d) : Node(d) {
        d.init();
    }

    MappingAction::MappingAction(Node *parent, const std::string &key, const Property &value,
                                 const Property &oldValue)
        : PropertyAction(MappingAssign, parent, value, oldValue), m_key(key) {
    }

    MappingAction::~MappingAction() {
    }

    void MappingAction::write(OStream &stream) const {
        PropertyAction::write(stream);
        stream << m_key;
        v.write(stream);
        oldv.write(stream);
    }

    Action *MappingAction::clone() const {
        return new MappingAction(m_parent, m_key, v, oldv);
    }

    void MappingAction::execute(bool undo) {
        auto d = static_cast<MappingNode *>(m_parent)->d_func();
        d->setProperty_helper(m_key, undo ? oldv : v);
    }

}