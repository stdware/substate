#include "VectorNode.h"

namespace ss {

    VectorNode::~VectorNode() = default;

    bool VectorNode::insert(int index, const ArrayView<std::shared_ptr<Node>> &nodes) {
        // TODO: implement
        return false;
    }

    bool VectorNode::move(int index, int count, int dest) {
        // TODO: implement
        return false;
    }

    bool VectorNode::remove(int index, int count) {
        // TODO: implement
        return false;
    }

    void VectorNode::write(std::ostream &os) const {
        // TODO: implement
    }

    void VectorNode::read(std::istream &is, NodeReader &nr) {
        // TODO: implement
    }

    std::shared_ptr<Node> VectorNode::clone(bool copyId) const {
        // TODO: implement
        return {};
    }

    void VectorNode::propagateChildren(const std::function<void(Node *)> &func) {
        // TODO: implement
    }

}