#include "StorageEngine.h"

#include "Model.h"

namespace ss {

    StorageEngine::~StorageEngine() = default;

    void StorageEngine::setup(Model *model) {
        _model = model;
    }

    void StorageEngine::prepare() {
    }

    void StorageEngine::abort() {
    }

    void StorageEngine::reset() {
        // Skip removing index when deleting item to speed up
        _model->_clearing = true;

        // Remove root item
        _model->_root.reset();

        _model->_clearing = false;

        _idMap.clear();
        _maxId = 0;
    }

    size_t StorageEngine::addId(Node *node, size_t id) {
        size_t newId = id > 0 ? (_maxId = std::max(_maxId, id), id) : (++_maxId);
        _idMap.insert(std::make_pair(newId, node));
        return newId;
    }

}