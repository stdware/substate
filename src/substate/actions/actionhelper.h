#ifndef ACTIONHELPER_H
#define ACTIONHELPER_H

#include <substate/action.h>

namespace Substate {

    class SUBSTATE_EXPORT ActionHelper {
    public:
        static inline void clean(Action *action);
        static inline void execute(Action *action, bool undo);
    };

    inline void ActionHelper::clean(Action *action) {
        action->clean();
    }

    inline void ActionHelper::execute(Action *action, bool undo) {
        action->execute(undo);
    }

}

#endif // ACTIONHELPER_H
