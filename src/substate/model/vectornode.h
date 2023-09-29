#ifndef VECTORNODE_H
#define VECTORNODE_H

#include <vector>

#include <substate/node.h>

namespace Substate {

    class VectorMoveAction;

    class VectorInsDelAction;

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

    protected:
        Node *clone(bool user) const override;

        void childDestroyed(Node *node) override;

    protected:
        VectorNode(VectorNodePrivate &d);

        friend class VectorMoveAction;
        friend class VectorInsDelAction;
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

    class SUBSTATE_EXPORT VectorAction : public NodeAction {
    public:
        VectorAction(Type type, Node *parent, int index);
        ~VectorAction();

    public:
        inline int index() const;

    public:
        int m_index;
    };

    inline int VectorAction::index() const {
        return m_index;
    }

    class SUBSTATE_EXPORT VectorMoveAction : public VectorAction {
    public:
        VectorMoveAction(Node *parent, int index, int count, int dest);
        ~VectorMoveAction();

    public:
        Action *clone() const override;
        void execute(bool undo) override;

    public:
        inline int count() const;
        inline int destination() const;

    protected:
        int cnt, dest;
    };

    inline int VectorMoveAction::count() const {
        return cnt;
    }

    inline int VectorMoveAction::destination() const {
        return dest;
    }

    class SUBSTATE_EXPORT VectorInsDelAction : public VectorAction {
    public:
        VectorInsDelAction(Type type, Node *parent, int index, const std::vector<Node *> &children);
        ~VectorInsDelAction();

    public:
        Action *clone() const override;
        void execute(bool undo) override;

        virtual void virtual_hook(int id, void *data) override;

    public:
        inline const std::vector<Node *> &children() const;

    protected:
        std::vector<Node *> m_children;
    };

    inline const std::vector<Node *> &VectorInsDelAction::children() const {
        return m_children;
    }

}

#endif // VECTORNODE_H
