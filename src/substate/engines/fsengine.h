#ifndef FSENGINE_H
#define FSENGINE_H

#include <substate/memengine.h>

namespace Substate {

    class FileSystemEnginePrivate;

    class SUBSTATE_EXPORT FileSystemEngine : public MemoryEngine {
    public:
        FileSystemEngine();
        ~FileSystemEngine();

    protected:
        FileSystemEngine(FileSystemEnginePrivate &d);
    };

}

#endif // FSENGINE_H
