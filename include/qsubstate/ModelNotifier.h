// Copyright (C) 2022-2025 Stdware Collections (https://www.github.com/stdware)
// SPDX-License-Identifier: Apache-2.0

#ifndef QSUBSTATE_MODELNOTIFIER_H
#define QSUBSTATE_MODELNOTIFIER_H

#include <memory>

#include <QtCore/QObject>

#include <substate/Action.h>

#include <qsubstate/qsubstate_global.h>

namespace ss {

    class Model;

    class Node;

    /// Emits the notifications of a Model as Qt signals. See ModelObserver for the occasions and
    /// the arguments.
    ///
    /// The notifier registers itself with the model at construction and removes itself at
    /// destruction, which must precede the destruction of the model. A slot must not modify the
    /// model.
    ///
    /// \note The pointers passed by the signals are valid only during the emission, therefore
    ///       only direct connections are supported.
    class QSUBSTATE_EXPORT ModelNotifier : public QObject {
        Q_OBJECT
    public:
        explicit ModelNotifier(Model *model, QObject *parent = nullptr);
        ~ModelNotifier();

        inline Model *model() const;

    Q_SIGNALS:
        void actionAboutToApply(const ss::Action *action, ss::Action::Operation operation);
        void actionApplied(const ss::Action *action, ss::Action::Operation operation);
        void stepChanged(int step);
        void nodeAboutToBeDestroyed(ss::Node *node);
        void aboutToReset();
        void resetFinished();

    private:
        class Observer;

        Model *m_model;
        std::unique_ptr<Observer> m_observer;
    };

    inline Model *ModelNotifier::model() const {
        return m_model;
    }

}

#endif // QSUBSTATE_MODELNOTIFIER_H
