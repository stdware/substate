#ifndef SUBSTATE_VECTORNODE_P_H
#define SUBSTATE_VECTORNODE_P_H

#include <substate/VectorNode.h>

namespace ss {

    class SUBSTATE_EXPORT VectorNodePrivate {
    public:
        static inline bool validateArrayQueryArguments(int index, int size) {
            return index >= 0 && index <= size;
        }

        static inline bool validateArrayRemoveArguments(int index, int count, int size) {
            return (index >= 0 && index < size)           // index bound
                   && (count > 0 && count <= size - index) // count bound
                ;
        }

        static void insert_TX(VectorNode *q, int index,
                                   const ArrayView<std::shared_ptr<Node>> &nodes);

        static void remove_TX(VectorNode *q, int index, int count);

        static void move_TX(VectorNode *q, int index, int count, int dest);

        static void copy(VectorNode *dest, const VectorNode *src, bool copyId);
    };

}

#endif // SUBSTATE_VECTORNODE_P_H