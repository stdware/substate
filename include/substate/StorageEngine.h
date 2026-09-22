// Copyright (C) 2022-2025 Stdware Collections (https://www.github.com/stdware)
// SPDX-License-Identifier: Apache-2.0

#ifndef SUBSTATE_STORAGEENGINE_H
#define SUBSTATE_STORAGEENGINE_H

#include <string>
#include <map>
#include <vector>
#include <memory>
#include <unordered_map>

#include <substate/Node.h>
#include <substate/Action.h>

namespace ss {

    class Model;

    class StorageEnginePrivate;

    /// StorageEngine - Node and Action storage backend, could be memory, filesystem, database, etc.
    class SUBSTATE_EXPORT StorageEngine {
    public:
        StorageEngine() = default;
        virtual ~StorageEngine();

        StorageEngine(const StorageEngine &) = delete;
        StorageEngine &operator=(const StorageEngine &) = delete;

        StorageEngine(StorageEngine &&) = default;
        StorageEngine &operator=(StorageEngine &&) = default;

    public:
        inline Model *model() const;

        inline Node *indexOf(size_t id) const;

        /// Sets up the engine with a model, must set \c this->_model to \c model after this call.
        virtual void setup(Model *model);

        /// Prepare for transaction, this function is called when the model turns into transaction
        /// mode.
        virtual void prepare();

        /// Abort transaction, this function is called when the model turns into idle mode without
        /// any actions to commit.
        virtual void abort();

        /// Commit a list of actions with a message to the engine.
        virtual void commit(std::vector<std::unique_ptr<Action>> actions,
                            std::map<std::string, std::string> message) = 0;

        /// Executes undo or redo and updates engine's internal state.
        virtual void execute(bool undo) = 0;

        /// Resets the engine and model.
        virtual void reset();

        virtual int minimum() const = 0;
        virtual int maximum() const = 0;
        virtual int current() const = 0;
        virtual std::map<std::string, std::string> stepMessage(int step) const = 0;

    protected:
        std::unordered_map<size_t, SmartPtr<Node>> _nodeMap;
        size_t _maxId = 0;
        Model *_model = nullptr;

        friend class Model;
        friend class Node;
        friend class NodePrivate;
        friend class StorageEnginePrivate;
    };

    inline Model *StorageEngine::model() const {
        return _model;
    }

    inline Node *StorageEngine::indexOf(size_t id) const {
        auto it = _nodeMap.find(id);
        if (it == _nodeMap.end()) {
            return nullptr;
        }
        return it->second.get();
    }

}

#endif // SUBSTATE_STORAGEENGINE_H
