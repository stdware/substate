#ifndef NODE_H
#define NODE_H

#include <memory>
#include <string>

#include <substate/operation.h>
#include <substate/stream.h>

namespace Substate {

    class Model;

    class NodePrivate;

    class SUBSTATE_EXPORT Node {
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
        virtual ~Node();

        typedef Node *(*Factory)(IStream &);

    public:
        Node *parent() const;
        Model *model() const;

        Type type() const;
        int userType() const;

        virtual void write(OStream &stream) const = 0;
        virtual Node *clone() const = 0;

        static Node *read(IStream &stream);
        static bool registerFactory(int type, Factory fac);

    protected:
        virtual void childDestroyed(Node *node);

    protected:
        void addChild(Node *node);
        void removeChild(Node *node);
        
        void dispatch(Operation *op, bool done);

    protected:
        std::unique_ptr<NodePrivate> d_ptr;
        Node(NodePrivate &d);
    };

}

#endif // NODE_H
