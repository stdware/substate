#ifndef MODEL_H
#define MODEL_H

#include <substate/node.h>

namespace Substate {

    class Subscriber;

    class ModelPrivate;

    class SUBSTATE_EXPORT Model {
        SUBSTATE_DECL_PRIVATE(Model)
    public:
        Model();
        virtual ~Model();

    public:
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
        void commitTransaction();

    public:
        void addSubscriber(Subscriber *sub);
        void removeSubscriber(Subscriber *sub);

    protected:
        void dispatch(Operation *op, bool done);

    protected:
        std::unique_ptr<ModelPrivate> d_ptr;
        Model(ModelPrivate &d);

        friend class Node;
    };

    inline bool Model::inTransaction() const {
        return state() == Transaction;
    }

    inline bool Model::stepChanging() const {
        return state() & UndoRedoFlag;
    }

    class SUBSTATE_EXPORT Subscriber {
    public:
        Subscriber();
        virtual ~Subscriber();

    protected:
        virtual void operation(Operation *op, bool done) = 0;

    private:
        Model *m_model;

        friend class Model;
        friend class ModelPrivate;
    };

}

#endif // MODEL_H
