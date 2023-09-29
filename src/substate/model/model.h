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

        void beginTransaction();
        void abortTransaction();
        void commitTransaction(const Variant &message);

        void undo();
        void redo();

        bool isWritable() const;

    protected:
        void dispatch(Action *action, bool done) override;

    protected:
        Model(ModelPrivate &d);

        friend class Node;
        friend class NodePrivate;
    };

    inline bool Model::inTransaction() const {
        return state() == Transaction;
    }

    inline bool Model::stepChanging() const {
        return state() & UndoRedoFlag;
    }

}

#endif // MODEL_H
