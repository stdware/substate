#include "SheetNode.h"

namespace ss {

    SheetNode::~SheetNode() = default;

    int SheetNode::insert(const std::shared_ptr<Node> &node) {
        // TODO: implement
        return {};
    }

    bool SheetNode::remove(int id) {
        // TODO: implement
        return {};
    }

    void SheetNode::write(std::ostream &os) const {
        // TODO: implement
    }

    void SheetNode::read(std::istream &is, NodeReader &nr) {
        // TODO: implement
    }

    std::shared_ptr<Node> SheetNode::clone(bool copyId) const {
        // TODO: implement
        return {};
    }

    void SheetNode::propagateChildren(const std::function<void(Node *)> &func) {
        // TODO: implement
    }
}