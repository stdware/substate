#include "nodehelper.h"
#include "node_p.h"

namespace Substate {

    /*!
        \class NodeHelper
        \brief Node private functions provided for user derived node classes and non-node classes.
    */

    /*!
        \fn NodePrivate *NodeHelper::get(Node *node);

        Gets the private pointer of the node.
    */

    /*!
        \fn Node *NodeHelper::clone(Node *node, bool user);

        Clones the node. If \a user is true, the index will be followed.
    */

    /*!
        Sets the index of the node, this operation is dangerous.

        Call this function only if the node is a copy of the previously destroyed one.
    */
    void NodeHelper::setIndex(Node *node, int index) {
        node->d_func()->index = index;
    }

    /*!
        Set the node as managed.

        Call this function only if the node is a copy of the previously destroyed one.
    */
    void NodeHelper::setManaged(Node *node, bool managed) {
        node->d_func()->setManaged(managed);
    }

    /*!
        Sets the node and all of its children's model pointer to the specified one, and
        processes the node's index.

        If the index of a node is 0, the node is regarded as newly created by the user and will be
        assigned the next highest index. Otherwise, the node is regarded as a managed node which is
        deserialized from raw data.
    */
    void NodeHelper::propagateModel(Node *node, Model *model) {
        node->d_func()->propagateModel(model);
    }

    /*!
        Delete the node anyway.
    */
    void NodeHelper::forceDelete(Node *node) {
        if (!node)
            return;
        node->propagateChildren([](Node *item) {
            item->d_func()->allowDelete = true; //
        });
        delete node;
    }

}