// Copyright (C) 2022-2025 Stdware Collections (https://www.github.com/stdware)
// SPDX-License-Identifier: Apache-2.0

#ifndef SUBSTATE_FILESYSTEMSTORAGEENGINE_H
#define SUBSTATE_FILESYSTEMSTORAGEENGINE_H

#include <filesystem>

#include <substate/StandardStorageEngine.h>

namespace ss {

    class SUBSTATE_EXPORT FilesystemStorageEngine : public StandardStorageEngine {
    public:
        explicit FilesystemStorageEngine(std::unique_ptr<ActionIOInterface> io);
        ~FilesystemStorageEngine();

    public:
        // TODO

    protected:
        std::unique_ptr<ActionIOInterface> _io;

        virtual bool createWarningFile(const std::filesystem::path &dir);
    };

}

#endif // SUBSTATE_FILESYSTEMSTORAGEENGINE_H