#ifndef SUBSTATE_NODE_H
#define SUBSTATE_NODE_H

#include <memory>
#include <functional>
#include <iostream>

#include <substate/Notification.h>

namespace ss {

    class Model;

    class ModelPrivate;

    class NodePrivate;

    /// Node - Document information storage unit.
    /// \note The node should be created by \c std::make_shared instead of created by direct
    /// construction.
    class SUBSTATE_EXPORT Node : public NotificationObserver,
                                 public std::enable_shared_from_this<Node> {
    public:
        inline Node(int type);
        ~Node();

        enum Type {
            Bytes,
            Vector,
            Sheet,
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
        inline std::shared_ptr<Node> parent() const;
        inline Model *model() const;

        inline bool isFree() const;
        bool isDetached() const;
        bool isWritable() const;

        /// Clone the node without copying its id.
        inline std::shared_ptr<Node> clone() const;

        /// Execute \a func on this node and all its children.
        inline void propagate(const std::function<void(Node *)> &func);

    protected:
        void beginAction();
        void endAction();

        void addChild(Node *child);
        void removeChild(Node *child);

        /// Clone the node with option(s).
        /// \param copyId The id of the cloned node = \a copyId ? 0 : this->id().
        virtual std::shared_ptr<Node> clone(bool copyId) const = 0;

        /// Execute \a func on all children.
        virtual void propagateChildren(const std::function<void(Node *)> &func);

        void notified(Notification *n) override;

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

    inline std::shared_ptr<Node> Node::parent() const {
        return _parent ? _parent->shared_from_this() : nullptr;
    }

    inline Model *Node::model() const {
        return _model;
    }

    inline bool Node::isFree() const {
        return _state == Created;
    }

    inline std::shared_ptr<Node> Node::clone() const {
        return clone(false);
    }

    inline void Node::propagate(const std::function<void(Node *)> &func) {
        func(this);
        propagateChildren(func);
    }


    /// NodeReader - Node deserialize interface.
    class NodeReader {
    public:
        inline NodeReader(std::istream &is);
        virtual ~NodeReader() = default;

        inline std::istream &in() const;

        virtual std::shared_ptr<Node> readOne() const = 0;

    protected:
        std::istream &_in;
    };

    inline NodeReader::NodeReader(std::istream &in) : _in(in) {
    }

    inline std::istream &NodeReader::in() const {
        return _in;
    }


    /// NodeWriter - Node serialize interface.
    class NodeWriter {
    public:
        inline NodeWriter(std::ostream &os);
        virtual ~NodeWriter() = default;

        inline std::ostream &out() const;

        virtual void witeOne(const std::shared_ptr<Node> &node) const = 0;

    protected:
        std::ostream &_out;
    };

    inline NodeWriter::NodeWriter(std::ostream &os) : _out(os) {
    }

    inline std::ostream &NodeWriter::out() const {
        return _out;
    }

}

#endif // SUBSTATE_NODE_H