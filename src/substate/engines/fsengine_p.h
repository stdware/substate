#ifndef FSENGINE_P_H
#define FSENGINE_P_H

#include <substate/private/memengine_p.h>
#include <substate/fsengine.h>

namespace Substate {

    class SUBSTATE_EXPORT FileSystemEnginePrivate : public MemoryEnginePrivate {
        SUBSTATE_DECL_PUBLIC(FileSystemEngine)
    public:
        FileSystemEnginePrivate();
        ~FileSystemEnginePrivate();
        void init();

        int maxCheckPoints;
        std::filesystem::path dir;
    };

}

#endif // FSENGINE_P_H
