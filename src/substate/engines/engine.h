#ifndef ENGINE_H
#define ENGINE_H

#include <memory>

#include <substate/operation.h>
#include <substate/variant.h>

namespace Substate {

    class EnginePrivate;

    class SUBSTATE_EXPORT Engine {
        SUBSTATE_DECL_PRIVATE(Engine)
    public:
        virtual ~Engine();

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

        void beginTransaction();
        void abortTransaction();
        void commitTransaction(const Variant &message);

        void commitOperation(Operation *op);

        int minimum() const;
        int maximum() const;
        int current() const;

    protected:
        virtual void commited(const std::vector<Operation *> ops, const Variant &message) = 0;

    protected:
        void setMinimum(int value);
        void setMaximum(int value);
        void setCurrent(int value);

    protected:
        std::unique_ptr<EnginePrivate> d_ptr;
        Engine(EnginePrivate &d);
    };

}

#endif // ENGINE_H
