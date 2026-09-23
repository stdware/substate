// Copyright (C) 2022-2025 Stdware Collections (https://www.github.com/stdware)
// SPDX-License-Identifier: Apache-2.0

#ifndef SUBSTATE_MODEL_H
#define SUBSTATE_MODEL_H

#include <cstddef>
#include <cstdint>
#include <functional>
#include <map>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include <substate/Action.h>
#include <substate/Node.h>
#include <substate/StorageEngine.h>

namespace ss {

    class ModelObserver;

    class NodePrivate;

    /// A document tree with transactions and undo history.
    ///
    /// Every modification of a node in the tree occurs within a transaction and is recorded as an
    /// action. A committed transaction is one undo step, stored by the storage engine. Every
    /// application of an action is reported to the observers, see ModelObserver.
    class SUBSTATE_EXPORT Model {
    public:
        /// Creates a model with a MemoryStorageEngine.
        Model();

        explicit Model(std::unique_ptr<StorageEngine> storageEngine);
        ~Model();

        Model(const Model &) = delete;
        Model &operator=(const Model &) = delete;

        inline StorageEngine *storageEngine() const;

        inline Node *root() const;

        /// Returns the node with \a id, or \c nullptr if no such node exists. A node removed from
        /// the tree remains available until the action that owns it is discarded.
        Node *nodeById(std::uint64_t id) const;

        /// The number of nodes with an identifier: the nodes in the tree and the nodes owned by
        /// actions in the history, including their descendants.
        inline std::size_t nodeCount() const;

        /// Replaces the root within the current transaction. \a root must be free and without a
        /// parent, or \c nullptr.
        void setRoot(std::unique_ptr<Node> root);

        /// Discards the tree and the history, and installs \a root as the initial tree without
        /// creating an action. Identifiers are not reused after a reset.
        void reset(std::unique_ptr<Node> root = {});

        /// Installs \a root, which a Decoder decoded into this model, as the tree without creating
        /// an action, and raises the identifier counter to at least \a lastId. The model must have
        /// no tree, as after reset(). The history of the storage engine is kept, because the
        /// storage engine restores it before this call.
        ///
        /// \a lastId is the largest identifier ever assigned, as recorded by the storage engine,
        /// so that the identifiers of destroyed nodes are not reused.
        void restore(std::unique_ptr<Node> root, std::uint64_t lastId = 0);

        void beginTransaction();

        /// Reverts every action of the current transaction in reverse order and discards them.
        /// The tree is restored to its state before beginTransaction().
        void abortTransaction();

        /// Commits the current transaction to the storage engine as one step, unless it contains
        /// no action.
        void commitTransaction(std::map<std::string, std::string> message = {});

        inline bool inTransaction() const;

        bool canUndo() const;
        bool canRedo() const;
        void undo();
        void redo();

        int minimumStep() const;
        int maximumStep() const;
        int currentStep() const;
        std::map<std::string, std::string> stepMessage(int step) const;

        /// Registers \a observer, which must remain valid until it is removed or the model is
        /// destroyed. Observers are notified in the order of registration.
        void addObserver(ModelObserver *observer);

        void removeObserver(ModelObserver *observer);

    private:
        enum class State {
            Idle,
            Transaction,
            Undo,
            Redo,

            // The tree and the history are being discarded by reset() or the destructor. No
            // notification of destroyed nodes is emitted.
            Reset,
        };

        // Applies action for operation between the two notifications of the observers.
        void apply(Action &action, Action::Operation operation);

        // Emits nodeAboutToBeDestroyed() for node and its descendants.
        void aboutToDestroy(Node *node);

        void notify(const std::function<void(ModelObserver *)> &func);

        // The declaration order determines the destruction order required by docs/Design.md:
        // the history first, then the tree, and the index last, because every node removes
        // itself from the index when destroyed.
        std::unordered_map<std::uint64_t, Node *> m_index;
        std::uint64_t m_lastId = 0;
        std::unique_ptr<Node> m_root;
        std::unique_ptr<StorageEngine> m_storageEngine;
        std::vector<std::unique_ptr<Action>> m_actions;
        State m_state = State::Idle;
        std::vector<ModelObserver *> m_observers;

        // Whether an observer is being notified, during which the model must not be modified.
        bool m_notifying = false;

        friend class NodePrivate;
        friend class RootChangeAction;
    };

    inline StorageEngine *Model::storageEngine() const {
        return m_storageEngine.get();
    }

    inline Node *Model::root() const {
        return m_root.get();
    }

    inline std::size_t Model::nodeCount() const {
        return m_index.size();
    }

    inline bool Model::inTransaction() const {
        return m_state == State::Transaction;
    }

}

#endif // SUBSTATE_MODEL_H
