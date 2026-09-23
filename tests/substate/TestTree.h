#ifndef SUBSTATE_TESTS_TESTTREE_H
#define SUBSTATE_TESTS_TESTTREE_H

#include <memory>
#include <string>
#include <vector>

#include <substate/VectorNode.h>

/// A VectorNode that counts its live instances. The count is compared with the identifier index
/// of the model, which verifies that no node outlives its owner and that no owner is missing,
/// without a leak detector.
class CountingNode : public ss::VectorNode {
public:
    inline CountingNode() : VectorNode(User) {
        ++s_live;
    }

    inline ~CountingNode() {
        --s_live;
    }

    inline std::unique_ptr<ss::Node> clone() const override {
        auto node = std::make_unique<CountingNode>();
        node->cloneChildrenFrom(*this);
        return node;
    }

    inline CountingNode *child(int index) const {
        return static_cast<CountingNode *>(at(index));
    }

    /// The number of CountingNode objects that exist.
    static inline int live() {
        return s_live;
    }

private:
    static inline int s_live = 0;
};

/// Returns a free node with \a children free leaves.
inline std::unique_ptr<CountingNode> makeNode(int children = 0) {
    auto node = std::make_unique<CountingNode>();
    for (int i = 0; i < children; ++i) {
        node->append(std::make_unique<CountingNode>());
    }
    return node;
}

/// Returns a vector holding \a node, for the functions that insert several nodes.
inline std::vector<std::unique_ptr<ss::Node>> nodes(std::unique_ptr<ss::Node> node) {
    std::vector<std::unique_ptr<ss::Node>> result;
    result.push_back(std::move(node));
    return result;
}

/// The structure of the tree under \a node as text: the identifier of each node, followed by the
/// children in parentheses. An empty string represents an empty tree.
inline std::string dump(const ss::Node *node) {
    if (!node) {
        return {};
    }
    auto vector = static_cast<const ss::VectorNode *>(node);
    std::string out = std::to_string(node->id());
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
    return out;
}

#endif // SUBSTATE_TESTS_TESTTREE_H
