#ifndef SUBSTATE_FILESYSTEMSTORAGEENGINE_H
#define SUBSTATE_FILESYSTEMSTORAGEENGINE_H

#include <substate/StandardStorageEngine.h>

namespace ss {

    class SUBSTATE_EXPORT FilesystemStorageEngine : public StandardStorageEngine {
    public:
        explicit FilesystemStorageEngine(std::unique_ptr<NodeReader> nr,
                                         std::unique_ptr<ActionReader> ar);
        ~FilesystemStorageEngine();

    public:
        // TODO

    protected:
        std::unique_ptr<NodeReader> _nr;
        std::unique_ptr<ActionReader> _ar;
    };

}

#endif // SUBSTATE_FILESYSTEMSTORAGEENGINE_H