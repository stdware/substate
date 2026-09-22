// Copyright (C) 2022-2025 Stdware Collections (https://www.github.com/stdware)
// SPDX-License-Identifier: Apache-2.0

#ifndef SUBSTATE_SHEETNODE_H
#define SUBSTATE_SHEETNODE_H

#include <set>
#include <unordered_map>

#include <substate/Node.h>
#include <substate/Action.h>

namespace ss {

    class SheetAction;

    class SheetNodePrivate;

    /// SheetNode - Auto-incrementing ID map data structure.
    class SUBSTATE_EXPORT SheetNode : public Node {
    public:
        inline explicit SheetNode(int type = Sheet);
        ~SheetNode();

    public:
        int insert(NodePtr node);
        bool remove(int id);
        inline Node *at(int id) const;
        inline const std::unordered_map<int, NodePtr> &data() const;
        inline int count() const;
        inline int size() const;

    protected:
        NodePtr clone(bool copyId) const override;
        void propagateChildren(const std::function<void(Node *)> &func) override;

        std::unordered_map<int, NodePtr> _sheet;
        int _maxId = 0;

        friend class SheetNodePrivate;
        friend class SheetAction;
    };

    inline SheetNode::SheetNode(int type) : Node(type) {
    }

    inline Node *SheetNode::at(int id) const {
        auto it = _sheet.find(id);
        if (it == _sheet.end()) {
            return nullptr;
        }
        return it->second.get();
    }

    inline const std::unordered_map<int, NodePtr> &SheetNode::data() const {
        return _sheet;
    }

    inline int SheetNode::count() const {
        return size();
    }

    inline int SheetNode::size() const {
        return int(_sheet.size());
    }


    /// SheetAction - Action for \c SheetNode operations.
    class SUBSTATE_EXPORT SheetAction : public NodeAction {
    public:
        inline SheetAction(Type type, Node *parent, int id, NodePtr child);
        ~SheetAction() = default;

    public:
        void queryNodes(bool inserted, const std::function<void(const NodePtr &)> &add) override;
        void execute(bool undo) override;

    public:
        inline int id() const;
        inline Node *child() const;

    protected:
        int _id;
        NodePtr _child;
    };

    inline SheetAction::SheetAction(Type type, Node *parent, int id, NodePtr child)
        : NodeAction(type, parent), _id(id), _child(std::move(child)) {
    }

    inline int SheetAction::id() const {
        return _id;
    }

    inline Node *SheetAction::child() const {
        return _child.get();
    }

}

#endif // SUBSTATE_SHEETNODE_H
