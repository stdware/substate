#include "FilesystemStorageEngine.h"

namespace ss {

    FilesystemStorageEngine::FilesystemStorageEngine(
        std::unique_ptr<ActionIOInterface> io)
        : _io(std::move(io)) {
    }

    FilesystemStorageEngine::~FilesystemStorageEngine() = default;

    bool FilesystemStorageEngine::createWarningFile(const std::filesystem::path &dir) {
        return false;
    }

}