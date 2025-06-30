#ifndef SUBSTATE_MODEL_H
#define SUBSTATE_MODEL_H

#include <vector>
#include <map>
#include <string>
#include <memory>

#include <substate/Notification.h>
#include <substate/Node.h>
#include <substate/Action.h>

namespace ss {

    class StorageEngine;

    class StandardStorageEngine;

    class ModelPrivate;

    /// Model - Document model and undo/redo manager.
    class SUBSTATE_EXPORT Model : public NotificationObserver {
    public:
        explicit Model(std::unique_ptr<StorageEngine> storageEngine);
        ~Model();

        Model(const Model &) = delete;
        Model &operator=(const Model &) = delete;

    public:
        inline StorageEngine *storageEngine() const;

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

        inline State state() const;
        inline bool inTransaction() const;
        inline bool stepChanging() const;

        /// Return if the model is writable.
        /// \note The model is writable only when it's in \c Transaction state and no node holds the
        /// action lock.
        bool isWritable() const;

        /// Return the node of \a id.
        std::shared_ptr<Node> indexOf(size_t id) const;

        inline std::shared_ptr<Node> root() const;
        void setRoot(const std::shared_ptr<Node> &root);

        /// Reset the model to empty state.
        void reset();

        /// Enters the transaction state.
        void beginTransaction();
        void abortTransaction();
        void commitTransaction(std::map<std::string, std::string> message);

        std::map<std::string, std::string> stepMessage(int step) const;

        void undo();
        void redo();

        inline bool canUndo() const;
        inline bool canRedo() const;

        int minimumStep() const;
        int maximumStep() const;
        int currentStep() const;

    protected:
        void notified(Notification *n) override;

        Node *_lockedNode;
        std::shared_ptr<Node> _root;
        State _state = Idle;
        std::vector<std::unique_ptr<Action>> _txActions;
        std::unique_ptr<StorageEngine> _storageEngine;
        bool _clearing = false;

        friend class Node;
        friend class NodePrivate;
        friend class ModelPrivate;
        friend class StorageEngine;
        friend class StandardStorageEngine;
    };

    inline StorageEngine *Model::storageEngine() const {
        return _storageEngine.get();
    }

    inline Model::State Model::state() const {
        return _state;
    }

    inline bool Model::inTransaction() const {
        return state() == Transaction;
    }

    inline bool Model::stepChanging() const {
        return state() & UndoRedoFlag;
    }

    inline std::shared_ptr<Node> Model::root() const {
        return _root;
    }

    inline bool Model::canUndo() const {
        return currentStep() > minimumStep();
    }

    inline bool Model::canRedo() const {
        return currentStep() < maximumStep();
    }

}

#endif // SUBSTATE_MODEL_H