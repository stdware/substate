// Copyright (C) 2022-2025 Stdware Collections (https://www.github.com/stdware)
// SPDX-License-Identifier: Apache-2.0

#ifndef SUBSTATE_NODE_H
#define SUBSTATE_NODE_H

#include <memory>
#include <functional>
#include <iostream>

#include <substate/Notification.h>
#include <substate/SmartPtr.h>

namespace ss {

    class Model;

    class ModelPrivate;

    class NodePrivate;

    class Node;

    /// Node - Document information storage unit.
    /// \note TODO The node should be created by \c std::make_shared instead of created by direct
    /// construction.
    class SUBSTATE_EXPORT Node : public NotificationSubject {
    public:
        inline Node(int type);
        ~Node();

        enum Type {
            Bytes,
            Vector,
            Sheet,
            Mapping,
            User = 1024,
        };

        enum State {
            /// The node is just created.
            Created,
            /// The node is on the tree.
            Active,
            /// The node is removed by user operation.
            Detached,
        };

        inline int type() const;
        inline State state() const;
        inline size_t id() const;
        inline Node *parent() const;
        inline Model *model() const;

        inline bool isFree() const;
        bool isDetached() const;
        bool isWritable() const;

        /// Clone the node without copying its id.
        inline Node *clone() const;

        /// Execute \a func on all children.
        virtual void propagateChildren(const std::function<void(NodePtr &)> &func);

    protected:
        void beginAction();
        void endAction();

        void addChild(Node *child);
        void removeChild(Node *child);

        void notify(Notification *n) override;

    protected:
        int _type;
        State _state = Created;
        size_t _id = 0;
        Node *_parent;
        Model *_model = nullptr;

        friend class Model;
        friend class ModelPrivate;
        friend class NodePrivate;
    };

    inline Node::Node(int type) : _type(type) {
    }

    inline int Node::type() const {
        return _type;
    }

    inline Node::State Node::state() const {
        return _state;
    }

    inline size_t Node::id() const {
        return _id;
    }

    inline Node *Node::parent() const {
        return _parent;
    }

    inline Model *Node::model() const {
        return _model;
    }

    inline bool Node::isFree() const {
        return _state == Created;
    }

}

#endif // SUBSTATE_NODE_H
