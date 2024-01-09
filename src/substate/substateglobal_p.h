#ifndef SUBSTATEGLOBAL_P_H
#define SUBSTATEGLOBAL_P_H

#include <unordered_map>

#define SUBSTATE_FIND_DEFERRED_REFERENCE_NODE(ARGS, NODE, OUT)                                     \
    {                                                                                              \
        auto &map = *reinterpret_cast<const std::unordered_map<int, Node *> *>(ARGS[0]);           \
        int idx = int(reinterpret_cast<uintptr_t>(NODE));                                          \
        if (idx == 0) {                                                                            \
            OUT = nullptr;                                                                         \
        } else {                                                                                   \
            auto it = map.find(idx);                                                               \
            if (it == map.end()) {                                                                 \
                QMSETUP_WARNING("Deferred reference node of id %d not found", idx);                \
                *reinterpret_cast<bool *>(ARGS[1]) = false;                                        \
                return;                                                                            \
            }                                                                                      \
            OUT = it->second;                                                                      \
        }                                                                                          \
    }

namespace Substate {

    template <typename ForwardIterator>
    void deleteAll(ForwardIterator begin, ForwardIterator end) {
        while (begin != end) {
            delete *begin;
            ++begin;
        }
    }

    template <typename Container>
    inline void deleteAll(const Container &c) {
        deleteAll(c.begin(), c.end());
    }

}

#endif // SUBSTATEGLOBAL_P_H
