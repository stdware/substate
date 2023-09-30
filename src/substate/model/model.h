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

        void reset();

        void beginTransaction();
        void abortTransaction();
        void commitTransaction(const Engine::StepMessage &message);

        void undo();
        void redo();

        inline bool canUndo() const;
        inline bool canRedo() const;

        int minimumStep() const;
        int maximumStep() const;
        int currentStep() const;

    public:
        void dispatch(Notification *n) override;

    protected:
        Model(ModelPrivate &d);

        friend class Engine;
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

    inline bool Model::canUndo() const {
        return currentStep() > minimumStep();
    }

    inline bool Model::canRedo() const {
        return currentStep() < maximumStep();
    }

}

#endif // MODEL_H
