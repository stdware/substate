#include <iostream>
#include <utility>
#include <vector>
#include <charconv>
#include <cstdlib>
#include <string_view>
#include <map>
#include <filesystem>

#include <substate/model.h>
#include <substate/fsengine.h>
#include <substate/vectornode.h>
#include <substate/mappingnode.h>
#include <substate/bytesnode.h>
#include <substate/sheetnode.h>
#include <substate/structnode.h>

using namespace Substate;

static const char *nodeType2str(Node::Type type) {
    switch (type) {
        case Node::Bytes: {
            return "Bytes";
        }
        case Node::Vector: {
            return "Vector";
        }
        case Node::Mapping: {
            return "Mapping";
        }
        case Node::Sheet: {
            return "Sheet";
        }
        case Node::Struct: {
            return "Struct";
        }
        default:
            break;
    }
    return nullptr;
}

static void printNode(const std::string &msg, Node *node) {
    static int indent = 0;
    static const auto &printIndent = []() { printf("%s", std::string(indent, ' ').data()); };

    // Print name
    printIndent();
    if (!msg.empty())
        printf("%s ", msg.data());
    printf("%s\n", nodeType2str(static_cast<Node::Type>(node->type())));

    indent += 2;

    // Print contents
    switch (node->type()) {
        case Node::Bytes: {
            auto n = static_cast<BytesNode *>(node);
            printIndent();
            printf("bytes: %s\n", n->data());
            break;
        }
        case Node::Vector: {
            auto n = static_cast<VectorNode *>(node);
            for (int i = 0; i < n->count(); ++i) {
                auto child = n->at(i);
                printNode(std::to_string(i), child);
            }
            break;
        }
        case Node::Mapping: {
            auto n = static_cast<MappingNode *>(node);
            const auto &data = n->data();
            for (const auto &item : data) {
                auto &key = item.first;
                auto &child = item.second;
                if (child.isVariant()) {
                    printIndent();
                    printf("%s variant: %s\n", key.data(), child.variant().toString().data());
                } else {
                    printNode(key, child.node());
                }
            }
            break;
        }
        case Node::Sheet: {
            auto n = static_cast<SheetNode *>(node);
            std::map<int, Node *> data{n->data().begin(), n->data().end()};
            for (const auto &item : std::as_const(data)) {
                printNode(std::to_string(item.first), item.second);
            }
            break;
        }
        case Node::Struct: {
            auto n = static_cast<StructNode *>(node);
            for (int i = 0; i < n->size(); ++i) {
                auto val = n->at(i);
                if (val.isVariant()) {
                    printIndent();
                    printf("%d variant: %s\n", i, val.variant().toString().data());
                } else {
                    printNode(std::to_string(i), val.node());
                }
            }
            break;
        }
        default:
            break;
    }

    indent -= 2;
}

int main(int argc, char *argv[]) {
    if (argc > 1) {
        auto engine = new FileSystemEngine();
        if (!engine->recover("model")) {
            printf("Recover failed.\n");
            return 0;
        }

        Model model(engine);
        printNode({}, model.root());
        return 0;
    }

    // Execute
    auto engine = new FileSystemEngine();
    std::filesystem::create_directories("model");
    engine->start("model");

    Model model(engine);

    auto rootNode = new MappingNode();
    rootNode->setProperty("1", Variant("a"));

    model.beginTransaction();
    model.setRoot(rootNode);
    model.commitTransaction({});

    model.beginTransaction();
    rootNode->setProperty("2", Variant("b"));
    model.commitTransaction({});

    model.beginTransaction();
    rootNode->setProperty("2", Variant("c"));
    model.commitTransaction({});

    model.undo();

    printNode({}, rootNode);

    return 0;
}