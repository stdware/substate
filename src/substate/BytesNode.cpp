#include "BytesNode.h"

namespace ss {

    BytesNode::~BytesNode() = default;

    void BytesNode::insert(int index, std::vector<char> data) {
    }

    void BytesNode::remove(int index, int size) {
    }

    void BytesNode::replace(int index, std::vector<char> data) {
    }

    BytesAction::~BytesAction() = default;

    void BytesAction::queryNodes(bool inserted,
                                 const std::function<void(const std::shared_ptr<Node> &)> &add) {
        (void) inserted;
        (void) add;
    }

    void BytesAction::execute(bool undo) {
    }

}