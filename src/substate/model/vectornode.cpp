#include "vectornode.h"
#include "vectornode_p.h"

namespace Substate {

    VectorNodePrivate::VectorNodePrivate(int type) : NodePrivate(type) {
    }

    VectorNodePrivate::~VectorNodePrivate() {
    }

    void VectorNodePrivate::init() {
    }

    VectorNode *VectorNodePrivate::read(IStream &stream) {
        return nullptr;
    }

    VectorNode::VectorNode() : VectorNode(*new VectorNodePrivate(Vector)) {
    }

    VectorNode::~VectorNode() {
    }

    bool VectorNode::insert(int index, const std::vector<Node *> &nodes) {
        return false;
    }

    bool VectorNode::move(int index, int count, int dest) {
        return false;
    }

    bool VectorNode::remove(int index, int count) {
        return false;
    }

    bool VectorNode::remove(Node *node) {
        return false;
    }

    Node *VectorNode::at(int index) const {
        Q_D(const VectorNode);
        return d->vector.at(index);
    }

    const std::vector<Node *> &VectorNode::data() const {
        Q_D(const VectorNode);
        return d->vector;
    }

    int VectorNode::indexOf(Node *node) const {
        Q_D(const VectorNode);
        auto it = std::find(d->vector.begin(), d->vector.end(), node);
        if (it == d->vector.end())
            return -1;
        return int(it - d->vector.begin());
    }

    int VectorNode::size() const {
        Q_D(const VectorNode);
        return int(d->vector.size());
    }

    void VectorNode::write(OStream &stream) const {
    }

    Node *VectorNode::clone() const {
        return nullptr;
    }

    void VectorNode::childDestroyed(Node *node) {
        remove(node);
    }

    VectorNode::VectorNode(VectorNodePrivate &d) : Node(d) {
        d.init();
    }

}