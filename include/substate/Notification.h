// Copyright (C) 2022-2025 Stdware Collections (https://www.github.com/stdware)
// SPDX-License-Identifier: Apache-2.0

#ifndef SUBSTATE_NOTIFICATION_H
#define SUBSTATE_NOTIFICATION_H

#include <substate/substate_global.h>

namespace ss {

    class Notification {
    public:
        enum Type {
            ActionAboutToTrigger,
            ActionTriggered,
            StepChange,
            AboutToReset,
        };

        explicit inline Notification(int type);
        virtual ~Notification() = default;

        inline int type() const;

    protected:
        int _type;
    };

    inline Notification::Notification(int type) : _type(type) {
    }

    inline int Notification::type() const {
        return _type;
    }

    class NotificationObserver {
    public:
        virtual ~NotificationObserver() = default;

        virtual void notified(Notification *n) = 0;
    };

}

#endif // SUBSTATE_NOTIFICATION_H