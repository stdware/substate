#include "mappingnode.h"
#include "mappingnode_p.h"

#include <algorithm>

namespace Substate {

    MappingNodePrivate::MappingNodePrivate(int type) : NodePrivate(type) {
    }

    MappingNodePrivate::~MappingNodePrivate() {
    }

    void MappingNodePrivate::init() {
    }

    MappingNode *MappingNodePrivate::read(IStream &stream) {
        return nullptr;
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
            return {const_cast<Variant *>(&it->second.variant)};
        return {it->second.node};
    }

    bool MappingNode::setProperty(const std::string &key, const Variant &value) {
        return false;
    }

    bool MappingNode::setProperty(const std::string &key, Node *node) {
        return false;
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
    }

    Node *MappingNode::clone(bool user) const {
        return nullptr;
    }

    void MappingNode::childDestroyed(Node *node) {
        Q_D(MappingNode);
        auto it = d->mappingIndexes.find(node);
        if (it == d->mappingIndexes.end())
            return;
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

}