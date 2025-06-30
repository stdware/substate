#ifndef SUBSTATE_NODE_H
#define SUBSTATE_NODE_H

#include <memory>
#include <functional>
#include <iostream>

#include <substate/Notification.h>
#include <substate/Action.h>
#include <substate/TwoPhaseObject.h>

namespace ss {

    class Model;

    class ModelPrivate;

    class NodePrivate;

    class NodeReader;

    /// Node - Document information storage unit.
    /// \note The node should be created by \c std::make_shared or \c make_node instead of created
    /// by direct construction.
    class SUBSTATE_EXPORT Node : public NotificationObserver,
                                 public std::enable_shared_from_this<Node> {
    public:
        inline Node(int type, int classType);
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
        inline int classType() const;
        inline size_t id() const;
        inline State state() const;
        inline std::shared_ptr<Node> parent() const;
        inline Model *model() const;

        inline bool isFree() const;
        bool isDetached() const;
        bool isWritable() const;

        /// Clone the node without copying its id.
        inline std::shared_ptr<Node> clone() const;

        /// Execute \a func on this node and all its children.
        inline void propagate(const std::function<void(Node *)> &func);

        /// Serialize the node.
        /// \note The \c type and \c classType should not be written, as they are used to determine
        /// the constructor and already written by the caller.
        virtual void write(std::ostream &os) const = 0;

        /// Deserialize the node.
        virtual void read(std::istream &is, NodeReader &nr) = 0;

    public:
        /// Serialize the action with the preceding \c type and \c classType enum.
        static void write(Node &node, std::ostream &os);

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
        int _classType; // metadata
        size_t _id = 0;
        State _state = Created;
        Node *_parent;
        Model *_model = nullptr;

        friend class Model;
        friend class ModelPrivate;
        friend class NodePrivate;
    };

    inline Node::Node(int type, int classType) : _type(type), _classType(classType) {
    }

    inline int Node::type() const {
        return _type;
    }

    inline int Node::classType() const {
        return _classType;
    }

    inline size_t Node::id() const {
        return _id;
    }

    inline Node::State Node::state() const {
        return _state;
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

    /// Create a \c Node instance of type \c T with arguments \a args.
    template <class T, class... Args>
    std::shared_ptr<T> make_node(Args &&...args) {
        static_assert(std::is_base_of<Node, T>::value, "T must derive from Node.");
        return std::make_shared<T>(std::forward<Args>(args)...);
    }

    /// NodeAction - Base action for node change.
    class SUBSTATE_EXPORT NodeAction : public Action {
    public:
        inline NodeAction(int type, const std::shared_ptr<Node> &parent);
        ~NodeAction();

        void write(std::ostream &os) const override;

        void read(std::istream &is) override;
        void initialize(const std::function<std::shared_ptr<Node>(size_t /*id*/)> &find) override;

        inline std::shared_ptr<Node> parent() const;

    protected:
        TwoPhaseObject<std::shared_ptr<Node>, size_t> _parent;
    };

    inline NodeAction::NodeAction(int type, const std::shared_ptr<Node> &parent)
        : Action(type), _parent(parent) {
    }

    inline std::shared_ptr<Node> NodeAction::parent() const {
        return _parent.value();
    }

    /// RootChangeAction - Action for model root change.
    class SUBSTATE_EXPORT RootChangeAction : public Action {
    public:
        inline RootChangeAction(const std::shared_ptr<Node> &oldRoot,
                                const std::shared_ptr<Node> &newRoot);
        ~RootChangeAction();

        std::unique_ptr<Action> clone(bool detach) const override;
        void write(std::ostream &os) const override;
        void queryNodes(bool inserted,
                        const std::function<void(const std::shared_ptr<Node> &)> &add) override;
        void execute(bool undo) override;

        void read(std::istream &is) override;
        void initialize(const std::function<std::shared_ptr<Node>(size_t /*id*/)> &find) override;

    public:
        inline std::shared_ptr<Node> root() const;
        inline std::shared_ptr<Node> oldRoot() const;

        /// For deserialization only.
        static inline std::unique_ptr<RootChangeAction> allocate();

    protected:
        inline RootChangeAction();

        TwoPhaseObject<std::shared_ptr<Node>, size_t> _oldRoot;
        TwoPhaseObject<std::shared_ptr<Node>, size_t> _newRoot;
    };

    inline RootChangeAction::RootChangeAction(const std::shared_ptr<Node> &oldRoot,
                                              const std::shared_ptr<Node> &newRoot)
        : Action(Action::RootChange), _oldRoot(oldRoot), _newRoot(newRoot) {
    }

    inline std::shared_ptr<Node> RootChangeAction::root() const {
        return _newRoot.value();
    }

    inline std::shared_ptr<Node> RootChangeAction::oldRoot() const {
        return _oldRoot.value();
    }

    inline std::unique_ptr<RootChangeAction> RootChangeAction::allocate() {
        return std::unique_ptr<RootChangeAction>(new RootChangeAction());
    }

    inline RootChangeAction::RootChangeAction()
        : Action(Action::RootChange), _oldRoot(0), _newRoot(0) {
    }

    class SUBSTATE_EXPORT NodeReader {
    public:
        virtual ~NodeReader() = default;

        std::shared_ptr<Node> readNode(std::istream &is);

    protected:
        /// Returns a new node deserialized from \a is with the \a type and \a classType.
        virtual std::shared_ptr<Node> readNode(int type, int classType, std::istream &is) const = 0;
    };

}

#endif // SUBSTATE_NODE_H