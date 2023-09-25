#include "sheetnode.h"
#include "sheetnode_p.h"

#include <algorithm>

namespace Substate {

    SheetNodePrivate::SheetNodePrivate(int type) : NodePrivate(type) {
    }

    SheetNodePrivate::~SheetNodePrivate() {
    }

    void SheetNodePrivate::init() {
    }

    SheetNode *SheetNodePrivate::read(Substate::IStream &stream) {
        return nullptr;
    }

    SheetNode::SheetNode() : SheetNode(*new SheetNodePrivate(Sheet)) {
    }

    SheetNode::~SheetNode() {
    }

    int SheetNode::insert(Node *node) {
        return 0;
    }

    bool SheetNode::remove(int id) {
        return false;
    }

    bool SheetNode::remove(Node *node) {
        return false;
    }

    Node *SheetNode::record(int id) {
        Q_D(const SheetNode);
        auto it = d->records.find(id);
        if (it == d->records.end())
            return nullptr;
        return it->second;
    }

    int SheetNode::indexOf(Node *node) const {
        Q_D(const SheetNode);
        auto it = d->recordIndexes.find(node);
        if (it == d->recordIndexes.end())
            return -1;
        return it->second;
    }

    std::vector<int> SheetNode::ids() const {
        Q_D(const SheetNode);
        std::vector<int> keys(d->records.size());
        std::transform(d->records.begin(), d->records.end(), keys.begin(),
                       [](const std::pair<int, Node *> &pair) {
                           return pair.first; //
                       });
        return keys;
    }

    const std::unordered_map<int, Node *> SheetNode::data() const {
        Q_D(const SheetNode);
        return d->records;
    }

    int SheetNode::size() const {
        Q_D(const SheetNode);
        return int(d->records.size());
    }

    void SheetNode::write(OStream &stream) const {
    }

    Node *SheetNode::clone() const {
        return nullptr;
    }

    void SheetNode::childDestroyed(Node *node) {
        remove(node);
    }

    SheetNode::SheetNode(SheetNodePrivate &d) : Node(d) {
        d.init();
    }

}