#ifndef SUBSTATE_MODELAPI_H
#define SUBSTATE_MODELAPI_H

#include <memory>
#include <functional>

#include <substate/Model.h>
#include <substate/Node.h>

namespace ss {

    /// ModelApi - Provides access to private methods of \c Model and \c Node instances,
    /// don't use this class unless you know what you are doing.
    class ModelApi {
    public:
        /// Call \c Node 's protected \c clone method.
        static inline std::shared_ptr<Node> cloneNode(Node *node, bool copyId) {
            return node->clone(copyId);
        }

        /// Call \c Node 's protected \c propagate method.
        static inline void propagateNode(Node *node, const std::function<void(Node *)> &func) {
            node->propagate(func);
        }

        /// Sets the id of the node silently.
        static inline void setNodeId(Node *node, size_t id) {
            node->_id = id;
        }

    private:
        ModelApi() = delete;
        ~ModelApi() = delete;
    };

}

#endif // SUBSTATE_MODELAPI_H