#include "operation.h"

#include <mutex>
#include <shared_mutex>

namespace Substate {

    static std::shared_mutex factoryLock;

    static std::unordered_map<int, Operation::Factory> factoryManager;

    static inline Operation::Factory getFactory(int type) {
        std::shared_lock<std::shared_mutex> lock(factoryLock);
        auto it = factoryManager.find(type);
        if (it == factoryManager.end()) {
            throw std::runtime_error("Substate::Node: Unknown operation type " + std::to_string(type));
        }
        return it->second;
    }

    Operation::~Operation() {
    }

    Operation *Operation::read(IStream &stream) {
        int type;
        stream >> type;
        if (stream.fail())
            return nullptr;

        switch (type) {
            default:
                break;
        }

        return getFactory(type)(stream);
    }

    bool Operation::registerFactory(int type, Factory fac) {
        std::unique_lock<std::shared_mutex> lock(factoryLock);

        auto it = factoryManager.find(type);
        if (it == factoryManager.end())
            return false;

        factoryManager.insert(std::make_pair(type, fac));
        return true;
    }

}