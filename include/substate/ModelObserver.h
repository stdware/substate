// Copyright (C) 2022-2025 Stdware Collections (https://www.github.com/stdware)
// SPDX-License-Identifier: Apache-2.0

#ifndef SUBSTATE_MODELOBSERVER_H
#define SUBSTATE_MODELOBSERVER_H

#include <substate/Action.h>

namespace ss {

    class Node;

    /// The receiver of the notifications of a Model, registered by Model::addObserver().
    ///
    /// Every function has an empty default implementation. A notification function may read the
    /// model and its nodes but must not modify the model, which is checked by assertion. Free
    /// nodes may be modified.
    class SUBSTATE_EXPORT ModelObserver {
    public:
        ModelObserver();
        virtual ~ModelObserver();

        ModelObserver(const ModelObserver &) = delete;
        ModelObserver &operator=(const ModelObserver &) = delete;

        /// Called before \a action is applied for \a operation, within a transaction for
        /// Action::Execute and for the undo of an aborted transaction, and within undo() or redo()
        /// otherwise. The accessors of the action that take an Action::Operation describe the
        /// change for \a operation. Nodes that enter the model with the action receive their
        /// identifiers when the action is applied.
        virtual void actionAboutToApply(const Action &action, Action::Operation operation);

        /// Called after \a action is applied for \a operation.
        virtual void actionApplied(const Action &action, Action::Operation operation);

        /// Called after a commit that creates a step, after undo() and after redo(), with the
        /// current step.
        virtual void stepChanged(int step);

        /// Called before \a node is destroyed together with the action that owns it, because the
        /// transaction of the action is discarded or aborted. \a node is not in the tree. It is
        /// called for every node of the destroyed subtree, parents before children, while the
        /// whole subtree is intact. It is not called for the nodes destroyed by Model::reset() or
        /// by the destruction of the model.
        virtual void nodeAboutToBeDestroyed(Node *node);

        /// Called before Model::reset() discards the tree and the history, and before the model
        /// is destroyed.
        virtual void aboutToReset();

        /// Called after Model::reset() installs the new tree.
        virtual void resetFinished();
    };

}

#endif // SUBSTATE_MODELOBSERVER_H
