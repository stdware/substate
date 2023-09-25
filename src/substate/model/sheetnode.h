#ifndef SHEETNODE_H
#define SHEETNODE_H

#include <unordered_map>
#include <vector>

#include <substate/node.h>

namespace Substate {

    class SheetNodePrivate;

    class SUBSTATE_EXPORT SheetNode : public Node {
        SUBSTATE_DECL_PRIVATE(SheetNode)
    public:
        SheetNode();
        ~SheetNode();

    public:
        int insert(Node *node);
        bool remove(int id);
        bool remove(Node *node);
        Node *record(int id);
        int indexOf(Node *node) const;
        std::vector<int> ids() const;
        const std::unordered_map<int, Node *> data() const;
        inline int count() const;
        int size() const;

    public:
        void write(OStream &stream) const override;
        Node *clone() const override;

    protected:
        void childDestroyed(Node *node) override;

    protected:
        SheetNode(SheetNodePrivate &d);
    };

    int SheetNode::count() const {
        return size();
    }

}

#endif // SHEETNODE_H
