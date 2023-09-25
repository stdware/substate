#ifndef VECTORNODE_H
#define VECTORNODE_H

#include <vector>

#include <substate/node.h>

namespace Substate {

    class VectorNodePrivate;

    class SUBSTATE_EXPORT VectorNode : public Node {
        SUBSTATE_DECL_PRIVATE(VectorNode)
    public:
        VectorNode();
        ~VectorNode();

    public:
        inline bool prepend(Node *node);
        inline bool prepend(const std::vector<Node *> &nodes);
        inline bool append(Node *node);
        inline bool append(const std::vector<Node *> &nodes);
        inline bool insert(int index, Node *node);
        inline bool removeOne(int index);
        bool insert(int index, const std::vector<Node *> &nodes);
        bool move(int index, int count, int dest);         // dest: destination index before move
        inline bool move2(int index, int count, int dest); // dest: destination index after move
        bool remove(int index, int count);
        bool remove(Node *node);
        Node *at(int index) const;
        const std::vector<Node *> &data() const;
        int indexOf(Node *node) const;
        inline int count() const;
        int size() const;

    public:
        void write(OStream &stream) const override;
        Node *clone() const override;

    protected:
        void childDestroyed(Node *node) override;

    protected:
        VectorNode(VectorNodePrivate &d);
    };

    bool VectorNode::prepend(Node *node) {
        return insert(0, node);
    }

    bool VectorNode::prepend(const std::vector<Node *> &nodes) {
        return insert(0, nodes);
    }

    bool VectorNode::append(Node *node) {
        return insert(size(), node);
    }

    bool VectorNode::append(const std::vector<Node *> &nodes) {
        return insert(size(), nodes);
    }

    bool VectorNode::insert(int index, Node *node) {
        return insert(size(), std::vector<Node *>({node}));
    }

    bool VectorNode::removeOne(int index) {
        return remove(index, 1);
    }

    bool VectorNode::move2(int index, int count, int dest) {
        return move(index, count, (dest <= index) ? dest : (dest + count));
    }

    int VectorNode::count() const {
        return size();
    }

}

#endif // VECTORNODE_H
