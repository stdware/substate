#ifndef MODEL_H
#define MODEL_H

#include <substate/engine.h>
#include <substate/node.h>

namespace Substate {

    class Subscriber;

    class ModelPrivate;

    class SUBSTATE_EXPORT Model : public Sender {
        SUBSTATE_DECL_PRIVATE(Model)
    public:
        Model();
        Model(Engine *engine);
        ~Model();

    public:
        Engine *engine() const;

        enum StateFlag {
            TransactionFlag = 1,
            UndoRedoFlag = 2,
            UndoFlag = 4,
            RedoFlag = 8,
        };

        enum State {
            Idle = 0,
            Transaction = TransactionFlag,
            Undo = UndoFlag | UndoRedoFlag,
            Redo = RedoFlag | UndoRedoFlag,
        };

        State state() const;
        inline bool inTransaction() const;
        inline bool stepChanging() const;

        bool isWritable() const;
        Node *indexOf(int index) const;

        Node *root() const;
        void setRoot(Node *node);

        void beginTransaction();
        void abortTransaction();
        void commitTransaction(const Variant &message);

        void undo();
        void redo();

    public:
        void dispatch(Notification *n) override;

    protected:
        Model(ModelPrivate &d);

        friend class Node;
        friend class NodePrivate;
        friend class NodeHelper;
        friend class RootChangeAction;
    };

    inline bool Model::inTransaction() const {
        return state() == Transaction;
    }

    inline bool Model::stepChanging() const {
        return state() & UndoRedoFlag;
    }

}

#endif // MODEL_H
