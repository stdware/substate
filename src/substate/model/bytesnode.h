#ifndef BYTESNODE_H
#define BYTESNODE_H

#include <substate/node.h>

namespace Substate {

    class BytesNodePrivate;

    class SUBSTATE_EXPORT BytesNode : public Node {
        SUBSTATE_DECL_PRIVATE(BytesNode)
    public:
        BytesNode();
        ~BytesNode();

    public:
        inline bool prepend(const char *buf, int size);
        inline bool append(const char *buf, int size);
        bool insert(int index, const char *buf, int size);
        bool remove(int index, int size);
        bool replace(int index, const char *buf, int size);
        inline bool truncate(int size);
        inline bool clear();
        const char *data() const;
        inline int count() const;
        int size() const;

    public:
        void write(OStream &stream) const override;

    protected:
        Node *clone(bool user) const override;
    };

    inline bool BytesNode::prepend(const char *buf, int size) {
        return insert(0, buf, size);
    }

    inline bool BytesNode::append(const char *buf, int size) {
        return insert(this->size(), buf, size);
    }

    inline bool BytesNode::truncate(int size) {
        return remove(size, this->size() - size);
    }

    inline bool BytesNode::clear() {
        return remove(0, size());
    }

    int BytesNode::count() const {
        return size();
    }

}

#endif // BYTESNODE_H
