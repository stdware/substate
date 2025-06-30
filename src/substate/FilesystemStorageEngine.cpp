#include "FilesystemStorageEngine.h"

namespace ss {

    FilesystemStorageEngine::FilesystemStorageEngine(std::unique_ptr<NodeReader> nr,
                                                     std::unique_ptr<ActionReader> ar)
        : _nr(std::move(nr)), _ar(std::move(ar)) {
    }

    FilesystemStorageEngine::~FilesystemStorageEngine() = default;
}