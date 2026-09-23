// Copyright (C) 2022-2025 Stdware Collections (https://www.github.com/stdware)
// SPDX-License-Identifier: Apache-2.0

#ifndef SUBSTATE_STORAGEENGINE_H
#define SUBSTATE_STORAGEENGINE_H

#include <map>
#include <string>

#include <substate/Transaction.h>

namespace ss {

    /// The history of a model.
    ///
    /// The engine stores committed transactions and decides where they are kept. The model
    /// executes them and emits the notifications. Steps are numbered from the creation of the
    /// model: step \c n is the state after the <tt>n</tt>-th committed transaction that is still
    /// reachable.
    ///
    /// An engine discards transactions only in two ways, as required by constraint 3 of
    /// docs/Design.md. Eviction discards the oldest executed transactions. Truncation discards
    /// all undone transactions, and occurs when a new transaction is committed. The nodes owned
    /// by a discarded transaction are destroyed with it.
    class SUBSTATE_EXPORT StorageEngine {
    public:
        StorageEngine();
        virtual ~StorageEngine();

        StorageEngine(const StorageEngine &) = delete;
        StorageEngine &operator=(const StorageEngine &) = delete;

        /// Takes ownership of \a transaction, which becomes the current step. Undone transactions
        /// are truncated first.
        virtual void commit(Transaction transaction) = 0;

        /// Returns the transaction of the current step and makes the previous step current, or
        /// returns \c nullptr if the current step is the minimum step. The transaction remains
        /// valid until the next commit or reset.
        virtual Transaction *stepBackward() = 0;

        /// Returns the transaction of the next step and makes it current, or returns \c nullptr
        /// if the current step is the maximum step. The transaction remains valid until the next
        /// commit or reset.
        virtual Transaction *stepForward() = 0;

        /// Discards all transactions and restarts the step numbers at 0.
        virtual void reset() = 0;

        /// The oldest step that can be reached by undo.
        virtual int minimumStep() const = 0;

        /// The newest step that can be reached by redo.
        virtual int maximumStep() const = 0;

        virtual int currentStep() const = 0;

        /// The message of the transaction that leads to \a step, or an empty map if \a step is
        /// out of range.
        virtual std::map<std::string, std::string> stepMessage(int step) const = 0;
    };

}

#endif // SUBSTATE_STORAGEENGINE_H
