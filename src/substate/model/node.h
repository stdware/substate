#ifndef NODE_H
#define NODE_H

#include <functional>
#include <memory>
#include <string>

#include <substate/action.h>
#include <substate/sender.h>
#include <substate/stream.h>

namespace Substate {

    class Model;

    class NodeHelper;

    class NodePrivate;

    class SUBSTATE_EXPORT Node : public Sender {
        SUBSTATE_DECL_PRIVATE(Node)
    public:
        enum Type {
            Bytes,
            Vector,
            Mapping,
            Sheet,
            User = 1024,
        };

        explicit Node(int type);
        ~Node();

        typedef Node *(*Factory)(IStream &);

    public:
        Node *parent() const;
        Model *model() const;
        int index() const;

        Type type() const;
        int userType() const;

        bool isFree() const;
        inline bool isObsolete() const;
        bool isManaged() const;
        bool isWritable() const;

        virtual void write(OStream &stream) const = 0;
        Node *clone() const;

        static Node *read(IStream &stream);
        static bool registerFactory(int type, Factory fac);

    protected:
        virtual Node *clone(bool user) const = 0;

        virtual void childDestroyed(Node *node);
        virtual void propagateChildren(const std::function<void(Node *)> &func);

    protected:
        void addChild(Node *node);
        void removeChild(Node *node);

        void beginAction();
        void endAction();

        void dispatch(Action *action, bool done) override;
        void propagate(const std::function<void(Node *)> &func);

    protected:
        Node(NodePrivate &d);

        friend class NodeHelper;
    };

    inline bool Node::isObsolete() const {
        return isManaged();
    }

}

#endif // NODE_H
