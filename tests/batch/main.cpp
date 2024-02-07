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

using namespace Substate;

int main(int argc, char *argv[]) {
    if (argc > 1) {
        // Recover
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

    printf("%s\n", rootNode->property("2").variant().toString().data());

    model.beginTransaction();
    rootNode->setProperty("2", Variant("c"));
    model.commitTransaction({});

    printf("%s\n", rootNode->property("2").variant().toString().data());

    model.undo();

    printf("%s\n", rootNode->property("2").variant().toString().data());
    printf("OK");
    return 0;
}