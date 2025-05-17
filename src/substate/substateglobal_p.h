#ifndef SUBSTATEGLOBAL_P_H
#define SUBSTATEGLOBAL_P_H

#include <unordered_map>

#define QM_D(Class) Class##Private *const d = d_func()
#define QM_Q(Class) Class *const q = q_func()

#define SUBSTATE_FIND_DEFERRED_REFERENCE_NODE(DATA, NODE, OUT)                                     \
    {                                                                                              \
        auto &map = *reinterpret_cast<const std::unordered_map<int, Node *> *>(DATA);              \
        int idx = int(reinterpret_cast<uintptr_t>(NODE));                                          \
        if (idx == 0) {                                                                            \
            OUT = nullptr;                                                                         \
        } else {                                                                                   \
            auto it = map.find(idx);                                                               \
            if (it == map.end()) {                                                                 \
                SUBSTATE_FATAL("Deferred reference node of id %d not found", idx);                  \
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
