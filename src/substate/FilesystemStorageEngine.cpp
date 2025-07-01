#include "FilesystemStorageEngine.h"

namespace ss {

    FilesystemStorageEngine::FilesystemStorageEngine(std::unique_ptr<NodeReader> nr,
                                                     std::unique_ptr<NodeWriter> nw,
                                                     std::unique_ptr<ActionReader> ar,
                                                     std::unique_ptr<ActionWriter> aw)
        : _nr(std::move(nr)), _nw(std::move(nw)), _ar(std::move(ar)), _aw(std::move(aw)) {
    }

    FilesystemStorageEngine::~FilesystemStorageEngine() = default;

    bool FilesystemStorageEngine::createWarningFile(const std::filesystem::path &dir) {
        return false;
    }

}