#ifndef BYTESNODE_H
#define BYTESNODE_H

#include <substate/action.h>
#include <substate/node.h>
#include <substate/bytearray.h>

namespace Substate {

    class BytesAction;

    class BytesNodePrivate;

    class SUBSTATE_EXPORT BytesNode : public Node {
        QMSETUP_DECL_PRIVATE(BytesNode)
    public:
        BytesNode(const std::string &name);
        ~BytesNode();

    public:
        inline bool prepend(const char *data, int size);
        inline bool append(const char *data, int size);
        bool insert(int index, const char *data, int size);
        bool remove(int index, int size);
        bool replace(int index, const char *data, int size);
        inline bool truncate(int size);
        inline bool clear();
        const char *data() const;
        inline int count() const;
        int size() const;

    public:
        void write(OStream &stream) const override;

    protected:
        Node *clone(bool user) const override;

        friend class BytesAction;
    };

    inline bool BytesNode::prepend(const char *data, int size) {
        return insert(0, data, size);
    }

    inline bool BytesNode::append(const char *data, int size) {
        return insert(this->size(), data, size);
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

    class SUBSTATE_EXPORT BytesAction : public NodeAction {
    public:
        BytesAction(Type type, Node *parent, int index, const ByteArray &bytes,
                    const ByteArray &oldBytes = {});
        ~BytesAction();

    public:
        void write(OStream &stream) const override;
        Action *clone() const override;
        void execute(bool undo) override;
        void virtual_hook(int id, void *data) override;

    public:
        inline int index() const;
        inline ByteArray bytes() const;
        inline ByteArray oldBytes() const;

    protected:
        int m_index;
        ByteArray b, oldb;
    };

    inline int BytesAction::index() const {
        return m_index;
    }

    inline ByteArray BytesAction::bytes() const {
        return b;
    }

    inline ByteArray BytesAction::oldBytes() const {
        return oldb;
    }

}

#endif // BYTESNODE_H
