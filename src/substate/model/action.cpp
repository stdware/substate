#include "action.h"

#include <shared_mutex>

#include "bytesnode_p.h"
#include "mappingnode_p.h"
#include "sheetnode_p.h"
#include "structnode_p.h"
#include "vectornode_p.h"

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

    Action *Action::read(IStream &stream) {
        int type;
        stream >> type;
        if (stream.fail())
            return nullptr;

        switch (type) {
            case BytesInsert:
            case BytesRemove:
            case BytesReplace:
                return readBytesAction(static_cast<Type>(type), stream);
            case SheetInsert:
            case SheetRemove:
                return readSheetAction(static_cast<Type>(type), stream);
            case VectorInsert:
            case VectorRemove:
                return readVectorInsDelAction(static_cast<Type>(type), stream);
            case VectorMove:
                return readVectorMoveAction(stream);
            case MappingAssign:
                return readMappingAction(stream);
            case StructAssign:
                return readStructAction(stream);
            case RootChange:
                return readRootChangeAction(stream);
            default:
                break;
        }

        return getFactory(type)(stream);
    }

    bool Action::registerFactory(int type, Factory fac) {
        std::unique_lock<std::shared_mutex> lock(factoryLock);

        auto it = factoryManager.find(type);
        if (it == factoryManager.end())
            return false;

        factoryManager.insert(std::make_pair(type, fac));
        return true;
    }

    void Action::detach() {
        virtual_hook(DetachHook, nullptr);
        s = Detached;
    }

    void Action::virtual_hook(int id, void *data) {
    }

    ActionNotification::ActionNotification(Notification::Type type, Action *action)
        : Notification(type), a(action) {
    }

    ActionNotification::~ActionNotification() {
    }

}