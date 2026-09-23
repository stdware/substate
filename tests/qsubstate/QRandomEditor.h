#ifndef QSUBSTATE_TESTS_QRANDOMEDITOR_H
#define QSUBSTATE_TESTS_QRANDOMEDITOR_H

#include <functional>
#include <memory>
#include <random>
#include <vector>

#include <QtCore/QString>
#include <QtCore/QVariant>
#include <QtTest/QTest>

#include <substate/Model.h>

#include "QTestTree.h"

/// Random modifications of trees of VectorNode, StructNode and MappingNode objects.
class QRandomEditor {
public:
    inline explicit QRandomEditor(unsigned seed) : m_random(seed) {
    }

    inline int uniform(int low, int high) {
        return std::uniform_int_distribution<int>(low, high)(m_random);
    }

    inline std::unique_ptr<ss::Node> subtree(int depth) {
        const int kind = uniform(0, 2);
        if (kind == 0) {
            auto node = std::make_unique<CountingStruct>();
            for (int i = 0; i < node->size(); ++i) {
                node->setAt(i, randomValue(depth));
            }
            return node;
        }
        if (kind == 1) {
            auto node = std::make_unique<CountingMapping>();
            const int entries = uniform(0, 2);
            for (int i = 0; i < entries; ++i) {
                node->setProperty(randomKey(), randomValue(depth));
            }
            return node;
        }
        auto node = std::make_unique<CountingNode>();
        const int children = depth > 0 ? uniform(0, 2) : 0;
        for (int i = 0; i < children; ++i) {
            node->append(subtree(depth - 1));
        }
        return node;
    }

    /// Performs one random modification within the current transaction. Every call creates
    /// exactly one action, so that a transaction is never empty.
    inline void modify(ss::Model &model) {
        auto root = model.root();
        const int kind = uniform(0, 21);
        if (!root || kind == 0) {
            model.setRoot(subtree(2));
            return;
        }

        Nodes all;
        collect(root, all);

        std::vector<ss::VectorNode *> removable;
        std::vector<ss::VectorNode *> movable;
        for (auto node : all.vectors) {
            if (node->size() > 0) {
                removable.push_back(node);
            }
            if (node->size() > 1) {
                movable.push_back(node);
            }
        }

        if (kind < 6 && !all.vectors.empty() && all.size() < 40) {
            auto parent = pick(all.vectors);
            parent->insert(uniform(0, parent->size()), subtree(uniform(0, 2)));
            return;
        }
        if (kind < 9 && !removable.empty()) {
            auto parent = pick(removable);
            const int index = uniform(0, parent->size() - 1);
            parent->remove(index, uniform(1, parent->size() - index));
            return;
        }
        if (kind < 10 && !movable.empty()) {
            auto parent = pick(movable);
            const int size = parent->size();
            const int index = uniform(0, size - 2);
            int destination = index;
            while (destination == index) {
                destination = uniform(0, size - 1);
            }
            parent->move(index, 1, destination);
            return;
        }
        if (kind < 15 && !all.structs.empty()) {
            auto node = pick(all.structs);
            const int index = uniform(0, node->size() - 1);
            node->setAt(index, changedValue(node->at(index)));
            return;
        }
        if (kind < 18 && transfer(all)) {
            return;
        }
        if (!all.mappings.empty()) {
            auto node = pick(all.mappings);
            const QString key = randomKey();
            QVERIFY(node->setProperty(key, changedValue(node->at(key))));
            return;
        }
        if (!all.structs.empty()) {
            auto node = pick(all.structs);
            node->setAt(0, changedValue(node->at(0)));
            return;
        }
        model.setRoot(subtree(2));
    }

private:
    struct Nodes {
        std::vector<ss::VectorNode *> vectors;
        std::vector<ss::StructNodeBase *> structs;
        std::vector<ss::MappingNode *> mappings;

        /// Every node except the root.
        std::vector<ss::Node *> children;

        inline size_t size() const {
            return vectors.size() + structs.size() + mappings.size();
        }
    };

    static inline bool isAncestorOrSelf(const ss::Node *node, const ss::Node *target) {
        for (auto current = target; current; current = current->parent()) {
            if (current == node) {
                return true;
            }
        }
        return false;
    }

    /// Transfers a random node to a random valid target: a VectorNode, an empty slot of a
    /// StructNode, or a missing key of a MappingNode. Returns false without an action if no
    /// valid target exists.
    inline bool transfer(const Nodes &all) {
        if (all.children.empty()) {
            return false;
        }
        auto node = pick(all.children);
        const auto valid = [&](const ss::Node *target) {
            return target != node->parent() && !isAncestorOrSelf(node, target);
        };

        std::vector<std::function<void()>> targets;
        for (auto target : all.vectors) {
            if (valid(target)) {
                targets.push_back([this, target, node] {
                    QVERIFY(target->transferIn(uniform(0, target->size()), node));
                });
            }
        }
        for (auto target : all.structs) {
            for (int i = 0; i < target->size(); ++i) {
                if (valid(target) && target->at(i).isEmpty()) {
                    targets.push_back([target, node, i] { QVERIFY(target->transferIn(i, node)); });
                }
            }
        }
        for (auto target : all.mappings) {
            const QString key = randomKey();
            if (valid(target) && !target->contains(key)) {
                targets.push_back([target, node, key] { QVERIFY(target->transferIn(key, node)); });
            }
        }
        if (targets.empty()) {
            return false;
        }
        pick(targets)();
        return true;
    }

    static inline void collect(ss::Node *node, Nodes &out) {
        if (node->parent()) {
            out.children.push_back(node);
        }
        if (auto vector = dynamic_cast<ss::VectorNode *>(node)) {
            out.vectors.push_back(vector);
            for (int i = 0; i < vector->size(); ++i) {
                collect(vector->at(i), out);
            }
        } else if (auto structNode = dynamic_cast<ss::StructNodeBase *>(node)) {
            out.structs.push_back(structNode);
            for (int i = 0; i < structNode->size(); ++i) {
                if (auto child = structNode->child(i)) {
                    collect(child, out);
                }
            }
        } else if (auto mapping = dynamic_cast<ss::MappingNode *>(node)) {
            out.mappings.push_back(mapping);
            for (const auto &key : mapping->keys()) {
                if (auto child = mapping->child(key)) {
                    collect(child, out);
                }
            }
        }
    }

    template <class T>
    inline T pick(const std::vector<T> &nodes) {
        return nodes[size_t(uniform(0, int(nodes.size()) - 1))];
    }

    inline QString randomKey() {
        return QString(QChar(u'a' + uniform(0, 3)));
    }

    /// A scalar value that differs from every previous one.
    inline QVariant uniqueVariant() {
        return QVariant(qint64(++m_serial));
    }

    inline ss::Property randomValue(int depth) {
        const int kind = uniform(0, 2);
        if (kind == 1) {
            return uniqueVariant();
        }
        if (kind == 2 && depth > 0) {
            return subtree(depth - 1);
        }
        return {};
    }

    /// A value that differs from \a current, so that the assignment creates an action.
    inline ss::Property changedValue(const ss::Property &current) {
        const int kind = uniform(current.isEmpty() ? 1 : 0, 2);
        if (kind == 1) {
            return uniqueVariant();
        }
        if (kind == 2) {
            return subtree(uniform(0, 1));
        }
        return {};
    }

    std::mt19937 m_random;
    qint64 m_serial = 0;
};

#endif // QSUBSTATE_TESTS_QRANDOMEDITOR_H
