#ifndef SUBSTATE_TESTS_TREEMIRROR_H
#define SUBSTATE_TESTS_TREEMIRROR_H

#include <algorithm>
#include <cstdint>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <vector>

#include <substate/BytesNode.h>
#include <substate/Model.h>
#include <substate/ModelObserver.h>
#include <substate/SheetNode.h>
#include <substate/VectorNode.h>

#include "TestTree.h"

/// An observer that reconstructs the tree of a model from the notifications alone, for the check
/// of their completeness and direction.
///
/// An inserted subtree is copied from the model when its insertion is reported. Every other change
/// is applied to the copy through the accessors of the action for the reported operation, and the
/// content removed from the copy is compared with the content that the action reports. The
/// identifiers of the copied nodes are kept until their destruction is reported.
class TreeMirror : public ss::ModelObserver {
public:
    inline explicit TreeMirror(const ss::Model &model) : m_step(model.currentStep()) {
        m_root = copy(model.root());
    }

    /// The structure of the copy, in the format of dump() of TestTree.h.
    inline std::string dump() const {
        return dumpNode(m_root.get());
    }

    /// The identifiers of the nodes that have entered the model and whose destruction has not
    /// been reported.
    inline const std::set<std::uint64_t> &liveIds() const {
        return m_live;
    }

    /// The step of the last stepChanged(), or the current step at construction if none.
    inline int step() const {
        return m_step;
    }

    /// The first inconsistency found, or an empty string if none.
    inline const std::string &error() const {
        return m_error;
    }

    inline void actionAboutToApply(const ss::Action &action,
                                   ss::Action::Operation operation) override {
        check(!m_pending, "nested actionAboutToApply()");
        m_pending = &action;
        if (action.type() == ss::Action::Transfer) {
            const auto &transfer = static_cast<const ss::TransferAction &>(action);
            for (auto node : transfer.nodes()) {
                check(node->parent() == transfer.source(operation), "transfer source");
            }
        }
    }

    inline void actionApplied(const ss::Action &action, ss::Action::Operation operation) override {
        check(m_pending == &action, "actionApplied() without actionAboutToApply()");
        m_pending = nullptr;

        switch (action.type()) {
            case ss::Action::RootChange: {
                const auto &a = static_cast<const ss::RootChangeAction &>(action);
                check((m_root ? m_root->id : 0) == idOf(a.oldRoot(operation)), "old root");
                m_root = copy(a.newRoot(operation));
                break;
            }
            case ss::Action::VectorInsert:
            case ss::Action::VectorRemove: {
                const auto &a = static_cast<const ss::VectorInsDelAction &>(action);
                auto &children = find(a.parent())->children;
                const auto first = children.begin() + a.index();
                if (a.isInsertion(operation)) {
                    std::vector<std::unique_ptr<MirrorNode>> copies;
                    for (auto child : a.children()) {
                        copies.push_back(copy(child));
                    }
                    children.insert(first, std::make_move_iterator(copies.begin()),
                                    std::make_move_iterator(copies.end()));
                } else {
                    for (size_t i = 0; i < a.children().size(); ++i) {
                        check(first[std::ptrdiff_t(i)]->id == a.children()[i]->id(),
                              "removed child");
                    }
                    children.erase(first, first + std::ptrdiff_t(a.children().size()));
                }
                break;
            }
            case ss::Action::VectorMove: {
                const auto &a = static_cast<const ss::VectorMoveAction &>(action);
                auto &children = find(a.parent())->children;
                auto begin = children.begin();
                const int index = a.index(operation);
                const int destination = a.destination(operation);
                if (destination < index) {
                    std::rotate(begin + destination, begin + index, begin + index + a.count());
                } else {
                    std::rotate(begin + index, begin + index + a.count(),
                                begin + destination + a.count());
                }
                break;
            }
            case ss::Action::SheetInsert:
            case ss::Action::SheetRemove: {
                const auto &a = static_cast<const ss::SheetInsDelAction &>(action);
                auto &sheet = find(a.parent())->sheet;
                if (a.isInsertion(operation)) {
                    check(sheet.count(a.key()) == 0, "inserted key");
                    sheet[a.key()] = copy(a.child());
                } else {
                    auto it = sheet.find(a.key());
                    check(it != sheet.end() && it->second->id == a.child()->id(), "removed key");
                    if (it != sheet.end()) {
                        sheet.erase(it);
                    }
                }
                break;
            }
            case ss::Action::BytesInsert:
            case ss::Action::BytesRemove: {
                const auto &a = static_cast<const ss::BytesInsDelAction &>(action);
                auto &bytes = find(a.parent())->bytes;
                const auto first = bytes.begin() + a.index();
                if (a.isInsertion(operation)) {
                    bytes.insert(first, a.bytes().begin(), a.bytes().end());
                } else {
                    check(std::equal(a.bytes().begin(), a.bytes().end(), first), "removed bytes");
                    bytes.erase(first, first + std::ptrdiff_t(a.bytes().size()));
                }
                break;
            }
            case ss::Action::BytesReplace: {
                const auto &a = static_cast<const ss::BytesReplaceAction &>(action);
                auto &bytes = find(a.parent())->bytes;
                const auto first = bytes.begin() + a.index();
                check(std::equal(a.oldBytes(operation).begin(), a.oldBytes(operation).end(), first),
                      "replaced bytes");
                std::copy(a.bytes(operation).begin(), a.bytes(operation).end(), first);
                break;
            }
            case ss::Action::Transfer:
                applyTransfer(static_cast<const ss::TransferAction &>(action), operation);
                break;
            default:
                check(false, "unknown action type");
                break;
        }
    }

    inline void stepChanged(int step) override {
        m_step = step;
    }

    inline void nodeAboutToBeDestroyed(ss::Node *node) override {
        check(!node->isAttached(), "destroyed node in the tree");
        check(m_live.erase(node->id()) == 1, "destroyed node unknown or reported twice");
        // Parents before children: the parent of a descendant has been reported already.
        check(!node->parent() || m_live.count(node->parent()->id()) == 0, "destruction order");
        // The node is intact, including the part of its most derived type.
        const int type = node->type();
        check((type == ss::Node::User && dynamic_cast<CountingNode *>(node)) ||
                  (type == ss::Node::User + 1 && dynamic_cast<CountingSheet *>(node)) ||
                  (type == ss::Node::User + 2 && dynamic_cast<CountingBytes *>(node)),
              "destroyed node not intact");
    }

private:
    struct MirrorNode {
        enum Kind {
            Vector,
            Sheet,
            Bytes,
        };

        std::uint64_t id = 0;
        Kind kind = Vector;
        std::vector<std::unique_ptr<MirrorNode>> children;
        std::map<int, std::unique_ptr<MirrorNode>> sheet;
        std::vector<char> bytes;
    };

    static inline std::uint64_t idOf(const ss::Node *node) {
        return node ? node->id() : 0;
    }

    inline void check(bool condition, const char *what) {
        if (!condition && m_error.empty()) {
            m_error = what;
        }
    }

    // Copies the subtree of node from the model and records its identifiers as live.
    inline std::unique_ptr<MirrorNode> copy(const ss::Node *node) {
        if (!node) {
            return nullptr;
        }
        auto result = std::make_unique<MirrorNode>();
        result->id = node->id();
        m_live.insert(node->id());
        if (auto vector = dynamic_cast<const ss::VectorNode *>(node)) {
            for (int i = 0; i < vector->size(); ++i) {
                result->children.push_back(copy(vector->at(i)));
            }
        } else if (auto sheet = dynamic_cast<const ss::SheetNode *>(node)) {
            result->kind = MirrorNode::Sheet;
            for (int key : sheet->keys()) {
                result->sheet[key] = copy(sheet->at(key));
            }
        } else if (auto bytes = dynamic_cast<const ss::BytesNode *>(node)) {
            result->kind = MirrorNode::Bytes;
            result->bytes.assign(bytes->data().begin(), bytes->data().end());
        }
        return result;
    }

    static inline MirrorNode *findIn(MirrorNode *node, std::uint64_t id) {
        if (!node || node->id == id) {
            return node;
        }
        for (const auto &child : node->children) {
            if (auto found = findIn(child.get(), id)) {
                return found;
            }
        }
        for (const auto &child : node->sheet) {
            if (auto found = findIn(child.second.get(), id)) {
                return found;
            }
        }
        return nullptr;
    }

    // Returns the copy of node, which must exist.
    inline MirrorNode *find(const ss::Node *node) {
        auto found = findIn(m_root.get(), node->id());
        check(found, "node missing in the copy");
        if (!found) {
            static MirrorNode placeholder;
            return &placeholder;
        }
        return found;
    }

    // Moves the copies of the transferred nodes from the source to the position of the nodes in
    // the target of the model.
    inline void applyTransfer(const ss::TransferAction &action, ss::Action::Operation operation) {
        const auto &nodes = action.nodes();
        for (auto node : nodes) {
            check(node->parent() == action.target(operation), "transfer target");
        }

        auto source = find(action.source(operation));
        std::vector<std::unique_ptr<MirrorNode>> moved;
        for (auto node : nodes) {
            const auto id = node->id();
            auto it = std::find_if(source->children.begin(), source->children.end(),
                                   [id](const auto &child) { return child->id == id; });
            if (it != source->children.end()) {
                moved.push_back(std::move(*it));
                source->children.erase(it);
                continue;
            }
            auto entry = std::find_if(source->sheet.begin(), source->sheet.end(),
                                      [id](const auto &child) { return child.second->id == id; });
            check(entry != source->sheet.end(), "transferred node missing in the source");
            if (entry != source->sheet.end()) {
                moved.push_back(std::move(entry->second));
                source->sheet.erase(entry);
            }
        }

        auto target = find(action.target(operation));
        if (auto vector = dynamic_cast<const ss::VectorNode *>(action.target(operation))) {
            int index = 0;
            while (index < vector->size() && vector->at(index) != nodes.front()) {
                ++index;
            }
            target->children.insert(target->children.begin() + index,
                                    std::make_move_iterator(moved.begin()),
                                    std::make_move_iterator(moved.end()));
        } else if (auto sheet = dynamic_cast<const ss::SheetNode *>(action.target(operation))) {
            for (int key : sheet->keys()) {
                if (sheet->at(key) == nodes.front() && !moved.empty()) {
                    target->sheet[key] = std::move(moved.front());
                }
            }
        }
    }

    static inline std::string dumpNode(const MirrorNode *node) {
        if (!node) {
            return {};
        }
        std::string out = std::to_string(node->id);
        switch (node->kind) {
            case MirrorNode::Vector:
                if (!node->children.empty()) {
                    out += '(';
                    for (size_t i = 0; i < node->children.size(); ++i) {
                        if (i > 0) {
                            out += ',';
                        }
                        out += dumpNode(node->children[i].get());
                    }
                    out += ')';
                }
                break;
            case MirrorNode::Sheet:
                if (!node->sheet.empty()) {
                    out += '{';
                    bool first = true;
                    for (const auto &child : node->sheet) {
                        if (!first) {
                            out += ',';
                        }
                        first = false;
                        out +=
                            '#' + std::to_string(child.first) + '=' + dumpNode(child.second.get());
                    }
                    out += '}';
                }
                break;
            case MirrorNode::Bytes:
                out += '[';
                for (char c : node->bytes) {
                    const auto value = static_cast<unsigned char>(c);
                    out += char('a' + (value >> 4));
                    out += char('a' + (value & 15));
                }
                out += ']';
                break;
        }
        return out;
    }

    std::unique_ptr<MirrorNode> m_root;
    std::set<std::uint64_t> m_live;
    const ss::Action *m_pending = nullptr;
    int m_step;
    std::string m_error;
};

#endif // SUBSTATE_TESTS_TREEMIRROR_H
