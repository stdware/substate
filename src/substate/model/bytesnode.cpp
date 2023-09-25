#include "bytesnode.h"
#include "bytesnode_p.h"

namespace Substate {

    BytesNodePrivate::BytesNodePrivate() : NodePrivate(Node::Bytes) {
    }

    BytesNodePrivate::~BytesNodePrivate() {
    }

    BytesNode *BytesNodePrivate::read(IStream &stream) {
        return nullptr;
    }

    BytesNode::BytesNode() : Node(*new BytesNodePrivate()) {
    }

    BytesNode::~BytesNode() {
    }

    bool BytesNode::insert(int index, const char *buf, int size) {
        return false;
    }

    bool BytesNode::remove(int index, int size) {
        return false;
    }

    bool BytesNode::replace(int index, const char *buf, int size) {
        return false;
    }

    const char *BytesNode::data() const {
        Q_D(const BytesNode);
        return d->byteArray.data();
    }

    int BytesNode::size() const {
        Q_D(const BytesNode);
        return int(d->byteArray.size());
    }

    void BytesNode::write(OStream &stream) const {
    }

    Node *BytesNode::clone() const {
        return nullptr;
    }

}