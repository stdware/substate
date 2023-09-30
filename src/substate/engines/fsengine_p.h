#ifndef FSENGINE_P_H
#define FSENGINE_P_H

#include <substate/private/memengine_p.h>
#include <substate/fsengine.h>

namespace Substate {

    class SUBSTATE_EXPORT FileSystemEnginePrivate : public MemEnginePrivate {
    public:
        FileSystemEnginePrivate();
        ~FileSystemEnginePrivate();
        void init();
    };

}

#endif // FSENGINE_P_H
