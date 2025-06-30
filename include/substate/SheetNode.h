#ifndef SUBSTATE_SHEETNODE_H
#define SUBSTATE_SHEETNODE_H

#include <set>
#include <unordered_map>

#include <substate/Node.h>

namespace ss {

    class SUBSTATE_EXPORT SheetNode : public Node {
    public:
        inline explicit SheetNode(int classType);
        ~SheetNode();

    public:
        int insert(const std::shared_ptr<Node> &node);
        bool remove(int id);
        inline std::shared_ptr<Node> at(int id) const;
        inline const std::unordered_map<int, std::shared_ptr<Node>> &data() const;
        inline int count() const;
        inline int size() const;

    public:
        void write(std::ostream &os) const override;
        void read(std::istream &is, NodeReader &nr) override;

    protected:
        std::shared_ptr<Node> clone(bool copyId) const override;
        void propagateChildren(const std::function<void(Node *)> &func) override;

        std::unordered_map<int, std::shared_ptr<Node>> _rec;
        std::set<int> _idSet;
    };

    inline SheetNode::SheetNode(int classType) : Node(Sheet, classType) {
    }

    inline std::shared_ptr<Node> SheetNode::at(int id) const {
        auto it = _rec.find(id);
        if (it == _rec.end()) {
            return {};
        }
        return it->second;
    }

    inline const std::unordered_map<int, std::shared_ptr<Node>> &SheetNode::data() const {
        return _rec;
    }

    inline int SheetNode::count() const {
        return size();
    }

    inline int SheetNode::size() const {
        return int(_rec.size());
    }

}

#endif // SUBSTATE_SHEETNODE_H