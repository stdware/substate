#ifndef SUBSTATE_TESTS_RANDOMEDITOR_H
#define SUBSTATE_TESTS_RANDOMEDITOR_H

#include <algorithm>
#include <memory>
#include <random>
#include <vector>

#include <substate/Model.h>

#include <boost/test/unit_test.hpp>

#include "TestTree.h"

/// Random modifications of trees of VectorNode, SheetNode and BytesNode objects, for the random
/// tests. Every modification creates exactly one action.
class RandomEditor {
public:
    inline explicit RandomEditor(unsigned seed) : m_random(seed) {
    }

    inline int uniform(int low, int high) {
        return std::uniform_int_distribution<int>(low, high)(m_random);
    }

    /// A random free subtree of VectorNode, SheetNode and BytesNode objects.
    inline std::unique_ptr<ss::Node> subtree(int depth) {
        const int children = depth > 0 ? uniform(0, 2) : 0;
        if (uniform(0, 4) == 0) {
            auto bytes = std::make_unique<CountingBytes>();
            bytes->append(randomBytes());
            return bytes;
        }
        if (uniform(0, 3) == 0) {
            auto sheet = std::make_unique<CountingSheet>();
            for (int i = 0; i < children; ++i) {
                sheet->insert(subtree(depth - 1));
            }
            return sheet;
        }
        auto node = std::make_unique<CountingNode>();
        for (int i = 0; i < children; ++i) {
            node->append(subtree(depth - 1));
        }
        return node;
    }

    /// Performs one random modification within the current transaction. Every call creates
    /// exactly one action, so that a transaction is never empty.
    inline void modify(ss::Model &model) {
        auto root = model.root();
        if (!root) {
            model.setRoot(subtree(2));
            return;
        }

        Containers all;
        collect(root, all);

        const int kind = uniform(0, 21);
        if (kind == 0) {
            // A null root occasionally, which leaves an empty tree.
            model.setRoot(uniform(0, 3) == 0 ? nullptr : subtree(2));
            return;
        }

        // The root is a BytesNode, and the tree has no node that can have children.
        if (all.size() == 0) {
            editBytes(pick(all.bytes));
            return;
        }

        if (kind < 9 && all.size() < 40) {
            insert(all);
            return;
        }

        Containers removable;
        std::vector<ss::VectorNode *> movable;
        for (auto node : all.vectors) {
            if (node->size() > 0) {
                removable.vectors.push_back(node);
            }
            if (node->size() > 1) {
                movable.push_back(node);
            }
        }
        for (auto node : all.sheets) {
            if (node->size() > 0) {
                removable.sheets.push_back(node);
            }
        }

        if (kind < 16 && removable.size() > 0) {
            remove(removable);
            return;
        }

        if (kind < 18 && !all.bytes.empty()) {
            editBytes(pick(all.bytes));
            return;
        }

        if (kind < 20 && transfer(all)) {
            return;
        }

        if (!movable.empty()) {
            auto parent = pick(movable);
            const int size = parent->size();
            const int index = uniform(0, size - 1);
            // At most size - 1 children, so that a destination other than index exists.
            const int count = uniform(1, std::min(size - index, size - 1));
            int destination = index;
            while (destination == index) {
                destination = uniform(0, size - count);
            }
            parent->move(index, count, destination);
            return;
        }

        insert(all);
    }

private:
    struct Containers {
        std::vector<ss::VectorNode *> vectors;
        std::vector<ss::SheetNode *> sheets;
        std::vector<ss::BytesNode *> bytes;

        // Every node except the root.
        std::vector<ss::Node *> children;

        // The number of nodes that can have children.
        inline size_t size() const {
            return vectors.size() + sheets.size();
        }
    };

    inline std::vector<char> randomBytes() {
        std::vector<char> result(size_t(uniform(1, 4)));
        for (auto &byte : result) {
            byte = char(uniform(0, 255));
        }
        return result;
    }

    // Inserts, removes or replaces bytes. A replacement may extend the array, which creates
    // a replacement and an insertion.
    inline void editBytes(ss::BytesNode *node) {
        const int size = node->size();
        const int kind = uniform(0, 2);
        if (kind == 0 || size == 0) {
            node->insert(uniform(0, size), randomBytes());
        } else if (kind == 1) {
            const int index = uniform(0, size - 1);
            node->remove(index, uniform(1, size - index));
        } else {
            node->replace(uniform(0, size), randomBytes());
        }
    }

    static inline void collect(ss::Node *node, Containers &out) {
        if (node->parent()) {
            out.children.push_back(node);
        }
        if (auto vector = dynamic_cast<ss::VectorNode *>(node)) {
            out.vectors.push_back(vector);
            for (int i = 0; i < vector->size(); ++i) {
                collect(vector->at(i), out);
            }
        } else if (auto sheet = dynamic_cast<ss::SheetNode *>(node)) {
            out.sheets.push_back(sheet);
            for (int key : sheet->keys()) {
                collect(sheet->at(key), out);
            }
        } else if (auto bytes = dynamic_cast<ss::BytesNode *>(node)) {
            out.bytes.push_back(bytes);
        }
    }

    template <class T>
    inline T pick(const std::vector<T> &nodes) {
        return nodes[size_t(uniform(0, int(nodes.size()) - 1))];
    }

    inline void insert(const Containers &all) {
        const int choice = uniform(0, int(all.size()) - 1);
        if (choice >= int(all.vectors.size())) {
            all.sheets[size_t(choice) - all.vectors.size()]->insert(subtree(uniform(0, 2)));
            return;
        }
        auto parent = all.vectors[size_t(choice)];
        std::vector<std::unique_ptr<ss::Node>> inserted;
        const int count = uniform(1, 2);
        for (int i = 0; i < count; ++i) {
            inserted.push_back(subtree(uniform(0, 2)));
        }
        parent->insert(uniform(0, parent->size()), std::move(inserted));
    }

    static inline bool isAncestorOrSelf(const ss::Node *node, const ss::Node *target) {
        for (auto current = target; current; current = current->parent()) {
            if (current == node) {
                return true;
            }
        }
        return false;
    }

    // Transfers a random node, or a range of consecutive children of a VectorNode, to a
    // random valid target. Returns false without an action if no valid target exists.
    inline bool transfer(const Containers &all) {
        if (all.children.empty()) {
            return false;
        }
        std::vector<ss::Node *> nodes{pick(all.children)};
        const auto parent = nodes.front()->parent();
        if (auto vector = dynamic_cast<ss::VectorNode *>(parent)) {
            int index = 0;
            while (vector->at(index) != nodes.front()) {
                ++index;
            }
            const int count = uniform(1, std::min(3, vector->size() - index));
            for (int i = 1; i < count; ++i) {
                nodes.push_back(vector->at(index + i));
            }
        }

        const auto valid = [&](const ss::Node *target) {
            if (target == parent) {
                return false;
            }
            for (auto node : nodes) {
                if (isAncestorOrSelf(node, target)) {
                    return false;
                }
            }
            return true;
        };
        std::vector<ss::VectorNode *> vectors;
        std::vector<ss::SheetNode *> sheets;
        for (auto node : all.vectors) {
            if (valid(node)) {
                vectors.push_back(node);
            }
        }
        // A SheetNode receives one node per transfer.
        if (nodes.size() == 1) {
            for (auto node : all.sheets) {
                if (valid(node)) {
                    sheets.push_back(node);
                }
            }
        }

        if (vectors.empty() && sheets.empty()) {
            return false;
        }
        const int choice = uniform(0, int(vectors.size() + sheets.size()) - 1);
        if (choice < int(vectors.size())) {
            auto target = vectors[size_t(choice)];
            BOOST_REQUIRE(target->transferIn(uniform(0, target->size()), nodes));
        } else {
            BOOST_REQUIRE(sheets[size_t(choice) - vectors.size()]->transferIn(nodes.front()) > 0);
        }
        return true;
    }

    inline void remove(const Containers &removable) {
        const int choice = uniform(0, int(removable.size()) - 1);
        if (choice >= int(removable.vectors.size())) {
            auto sheet = removable.sheets[size_t(choice) - removable.vectors.size()];
            BOOST_REQUIRE(sheet->remove(pick(sheet->keys())));
            return;
        }
        auto parent = removable.vectors[size_t(choice)];
        const int index = uniform(0, parent->size() - 1);
        parent->remove(index, uniform(1, parent->size() - index));
    }

    std::mt19937 m_random;
};

#endif // SUBSTATE_TESTS_RANDOMEDITOR_H
