#include "action.h"

#include <shared_mutex>

#include "model/bytesnode_p.h"
#include "model/mappingnode_p.h"
#include "model/sheetnode_p.h"
#include "model/vectornode_p.h"

namespace Substate {

    static std::shared_mutex factoryLock;

    static std::unordered_map<int, Action::Factory> factoryManager;

    static inline Action::Factory getFactory(int type) {
        std::shared_lock<std::shared_mutex> lock(factoryLock);
        auto it = factoryManager.find(type);
        if (it == factoryManager.end()) {
            throw std::runtime_error("Substate::Node: Unknown Action type " + std::to_string(type));
        }
        return it->second;
    }

    Action::~Action() {
    }

    Action *Action::read(IStream &stream, const std::unordered_map<int, Node *> &existingNodes) {
        int type;
        stream >> type;
        if (stream.fail())
            return nullptr;

        switch (type) {
            case BytesInsert:
            case BytesRemove:
            case BytesReplace:
                return readBytesAction(static_cast<Type>(type), stream, existingNodes);
            case SheetInsert:
            case SheetRemove:
                return readSheetAction(static_cast<Type>(type), stream, existingNodes);
            case VectorInsert:
            case VectorRemove:
                return readVectorInsDelAction(static_cast<Type>(type), stream, existingNodes);
            case VectorMove:
                return readVectorMoveAction(stream, existingNodes);
            case MappingSet:
                return readMappingAction(stream, existingNodes);
            case RootChange:
                return readRootChangeAction(stream, existingNodes);
            default:
                break;
        }

        return getFactory(type)(stream, existingNodes);
    }

    bool Action::registerFactory(int type, Factory fac) {
        std::unique_lock<std::shared_mutex> lock(factoryLock);

        auto it = factoryManager.find(type);
        if (it == factoryManager.end())
            return false;

        factoryManager.insert(std::make_pair(type, fac));
        return true;
    }

    void Action::virtual_hook(int id, void *data) {
    }

    ActionNotification::ActionNotification(Notification::Type type, Action *action)
        : Notification(type), a(action) {
    }

    ActionNotification::~ActionNotification() {
    }

}