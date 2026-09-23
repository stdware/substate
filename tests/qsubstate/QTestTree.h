#ifndef QSUBSTATE_TESTS_QTESTTREE_H
#define QSUBSTATE_TESTS_QTESTTREE_H

#include <memory>
#include <string>

#include <QtCore/QByteArray>
#include <QtCore/QString>

#include <qsubstate/MappingNode.h>
#include <qsubstate/StructNode.h>

#include "TestTree.h"

/// A StructNode with three slots that counts its live instances in LiveNodes.
class CountingStruct : public ss::StructNode<3> {
public:
    inline CountingStruct() : StructNode<3>(User + 3) {
        LiveNodes::created();
    }

    inline ~CountingStruct() {
        LiveNodes::destroyed();
    }

    inline std::unique_ptr<ss::Node> clone() const override {
        auto node = std::make_unique<CountingStruct>();
        node->cloneSlotsFrom(*this);
        return node;
    }
};

/// A MappingNode that counts its live instances in LiveNodes.
class CountingMapping : public ss::MappingNode {
public:
    inline CountingMapping() : MappingNode(User + 4) {
        LiveNodes::created();
    }

    inline ~CountingMapping() {
        LiveNodes::destroyed();
    }

    inline std::unique_ptr<ss::Node> clone() const override {
        auto node = std::make_unique<CountingMapping>();
        node->cloneEntriesFrom(*this);
        return node;
    }
};

/// Writes \a text as letters from \c a to \c p, two per byte of its UTF-8 form, so that the
/// result contains no digits.
inline std::string lettersOf(const QString &text) {
    std::string out;
    for (char c : text.toUtf8()) {
        const auto value = static_cast<unsigned char>(c);
        out += char('a' + (value >> 4));
        out += char('a' + (value & 15));
    }
    return out;
}

inline std::string qdump(const ss::Node *node);

/// The text form of a Property within qdump(): \c _ if empty, a quote followed by the letters of
/// the scalar value, or the text form of the child.
inline std::string qdump(const ss::Property &value) {
    switch (value.type()) {
        case ss::Property::Variant:
            return '\'' + lettersOf(value.variant().toString());
        case ss::Property::Child:
            return qdump(value.child());
        default:
            break;
    }
    return "_";
}

/// The structure of the tree under \a node as text, like dump() of TestTree.h, extended to the
/// node types of qsubstate. The slots of a StructNode are written in angle brackets, and the
/// entries of a MappingNode in braces as the letters of the key, \c =, and the value. Scalar
/// values and keys are written as letters, so that the only digits are identifiers.
inline std::string qdump(const ss::Node *node) {
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
                out += qdump(vector->at(i));
            }
            out += ')';
        }
    } else if (auto structNode = dynamic_cast<const ss::StructNodeBase *>(node)) {
        out += '<';
        for (int i = 0; i < structNode->size(); ++i) {
            if (i > 0) {
                out += ',';
            }
            out += qdump(structNode->at(i));
        }
        out += '>';
    } else if (auto mapping = dynamic_cast<const ss::MappingNode *>(node)) {
        out += '{';
        bool first = true;
        for (const auto &key : mapping->keys()) {
            if (!first) {
                out += ',';
            }
            first = false;
            out += lettersOf(key) + '=' + qdump(mapping->at(key));
        }
        out += '}';
    }
    return out;
}

#endif // QSUBSTATE_TESTS_QTESTTREE_H
