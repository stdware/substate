#ifndef SUBSTATE_TESTS_TESTTREE_H
#define SUBSTATE_TESTS_TESTTREE_H

#include <memory>
#include <string>

#include <substate/BytesNode.h>
#include <substate/SheetNode.h>
#include <substate/VectorNode.h>

/// The number of live objects of the counting node types of the tests. The count is compared with
/// the identifier index of the model, which verifies that no node outlives its owner and that no
/// owner is missing, without a leak detector.
class LiveNodes {
public:
    static inline int count() {
        return s_count;
    }

    static inline void created() {
        ++s_count;
    }

    static inline void destroyed() {
        --s_count;
    }

private:
    static inline int s_count = 0;
};

/// A VectorNode that counts its live instances in LiveNodes.
class CountingNode : public ss::VectorNode {
public:
    inline CountingNode() : VectorNode(User) {
        LiveNodes::created();
    }

    inline ~CountingNode() {
        LiveNodes::destroyed();
    }

    inline std::unique_ptr<ss::Node> clone() const override {
        auto node = std::make_unique<CountingNode>();
        node->cloneChildrenFrom(*this);
        return node;
    }

    inline CountingNode *child(int index) const {
        return static_cast<CountingNode *>(at(index));
    }

    /// The number of live objects of all counting node types.
    static inline int live() {
        return LiveNodes::count();
    }
};

/// A SheetNode that counts its live instances in LiveNodes.
class CountingSheet : public ss::SheetNode {
public:
    inline CountingSheet() : SheetNode(User + 1) {
        LiveNodes::created();
    }

    inline ~CountingSheet() {
        LiveNodes::destroyed();
    }

    inline std::unique_ptr<ss::Node> clone() const override {
        auto node = std::make_unique<CountingSheet>();
        node->cloneChildrenFrom(*this);
        return node;
    }
};

/// A BytesNode that counts its live instances in LiveNodes.
class CountingBytes : public ss::BytesNode {
public:
    inline CountingBytes() : BytesNode(User + 2) {
        LiveNodes::created();
    }

    inline ~CountingBytes() {
        LiveNodes::destroyed();
    }

    inline std::unique_ptr<ss::Node> clone() const override {
        auto node = std::make_unique<CountingBytes>();
        node->cloneDataFrom(*this);
        return node;
    }
};

/// Returns a free node with \a children free leaves.
inline std::unique_ptr<CountingNode> makeNode(int children = 0) {
    auto node = std::make_unique<CountingNode>();
    for (int i = 0; i < children; ++i) {
        node->append(std::make_unique<CountingNode>());
    }
    return node;
}

/// The structure of the tree under \a node as text. Each node is written as its identifier,
/// followed by the children of a VectorNode in parentheses, by the children of a SheetNode in
/// braces, each preceded by <tt>#key=</tt>, or by the bytes of a BytesNode in brackets. A byte is
/// written as two letters from \c a to \c p, one per half, so that the text contains no digits
/// other than identifiers and keys. An empty string represents an empty tree.
inline std::string dump(const ss::Node *node) {
    if (!node) {
        return {};
    }
    std::string out = std::to_string(node->id());
    if (auto vector = dynamic_cast<const ss::VectorNode *>(node)) {
        if (vector->size() > 0) {
            out += '(';
            for (int i = 0; i < vector->size(); ++i) {
                if (i > 0) {
                    out += ',';
                }
                out += dump(vector->at(i));
            }
            out += ')';
        }
    } else if (auto sheet = dynamic_cast<const ss::SheetNode *>(node)) {
        if (sheet->size() > 0) {
            out += '{';
            bool first = true;
            for (int key : sheet->keys()) {
                if (!first) {
                    out += ',';
                }
                first = false;
                out += '#' + std::to_string(key) + '=' + dump(sheet->at(key));
            }
            out += '}';
        }
    } else if (auto bytes = dynamic_cast<const ss::BytesNode *>(node)) {
        out += '[';
        for (char c : bytes->data()) {
            const auto value = static_cast<unsigned char>(c);
            out += char('a' + (value >> 4));
            out += char('a' + (value & 15));
        }
        out += ']';
    }
    return out;
}

#endif // SUBSTATE_TESTS_TESTTREE_H
