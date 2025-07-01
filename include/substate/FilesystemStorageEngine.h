// Copyright (C) 2022-2025 Stdware Collections (https://www.github.com/stdware)
// SPDX-License-Identifier: Apache-2.0

#ifndef SUBSTATE_FILESYSTEMSTORAGEENGINE_H
#define SUBSTATE_FILESYSTEMSTORAGEENGINE_H

#include <filesystem>

#include <substate/StandardStorageEngine.h>

namespace ss {

    class SUBSTATE_EXPORT FilesystemStorageEngine : public StandardStorageEngine {
    public:
        explicit FilesystemStorageEngine(std::unique_ptr<NodeReader> nr,
                                         std::unique_ptr<NodeWriter> nw,
                                         std::unique_ptr<ActionReader> ar,
                                         std::unique_ptr<ActionWriter> aw);
        ~FilesystemStorageEngine();

    public:
        // TODO

    protected:
        std::unique_ptr<NodeReader> _nr;
        std::unique_ptr<NodeWriter> _nw;
        std::unique_ptr<ActionReader> _ar;
        std::unique_ptr<ActionWriter> _aw;

        virtual bool createWarningFile(const std::filesystem::path &dir);
    };

}

#endif // SUBSTATE_FILESYSTEMSTORAGEENGINE_H