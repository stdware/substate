#ifndef SUBSTATE_VECTORNODE_H
#define SUBSTATE_VECTORNODE_H

#include <vector>

#include <substate/Node.h>
#include <substate/ArrayView.h>

namespace ss {

    class SUBSTATE_EXPORT VectorNode : public Node {
    public:
        inline explicit VectorNode(int classType);
        ~VectorNode();

    public:
        inline bool prepend(const std::shared_ptr<Node> &node);
        inline bool prepend(const ArrayView<std::shared_ptr<Node>> &nodes);
        inline bool append(const std::shared_ptr<Node> &node);
        inline bool append(const ArrayView<std::shared_ptr<Node>> &nodes);
        inline bool insert(int index, const std::shared_ptr<Node> &node);
        inline bool removeOne(int index);
        bool insert(int index, const ArrayView<std::shared_ptr<Node>> &nodes);
        bool move(int index, int count, int dest);         // dest: destination index before move
        inline bool move2(int index, int count, int dest); // dest: destination index after move
        bool remove(int index, int count);
        inline std::shared_ptr<Node> at(int index) const;
        inline ArrayView<std::shared_ptr<Node>> data() const;
        inline int count() const;
        inline int size() const;

    public:
        void write(std::ostream &os) const override;
        void read(std::istream &is, NodeReader &nr) override;

    protected:
        std::shared_ptr<Node> clone(bool copyId) const override;
        void propagateChildren(const std::function<void(Node *)> &func) override;

        std::vector<std::shared_ptr<Node>> _vec;
    };

    inline VectorNode::VectorNode(int classType) : Node(Vector, classType) {
    }

    inline bool VectorNode::prepend(const std::shared_ptr<Node> &node) {
        return insert(0, node);
    }

    inline bool VectorNode::prepend(const ArrayView<std::shared_ptr<Node>> &nodes) {
        return insert(0, nodes);
    }

    inline bool VectorNode::append(const std::shared_ptr<Node> &node) {
        return insert(size(), node);
    }

    inline bool VectorNode::append(const ArrayView<std::shared_ptr<Node>> &nodes) {
        return insert(size(), nodes);
    }

    inline bool VectorNode::insert(int index, const std::shared_ptr<Node> &node) {
        return insert(index, ArrayView<std::shared_ptr<Node>>(node));
    }

    inline bool VectorNode::removeOne(int index) {
        return remove(index, 1);
    }

    inline bool VectorNode::move2(int index, int count, int dest) {
        return move(index, count, (dest <= index) ? dest : (dest + count));
    }

    inline std::shared_ptr<Node> VectorNode::at(int index) const {
        return _vec.at(index);
    }

    inline ArrayView<std::shared_ptr<Node>> VectorNode::data() const {
        return _vec;
    }

    inline int VectorNode::count() const {
        return size();
    }

    inline int VectorNode::size() const {
        return int(_vec.size());
    }

}

#endif // SUBSTATE_VECTORNODE_H