project(test_qsubstate LANGUAGES CXX)

find_package(Qt6 COMPONENTS Core Test REQUIRED)

set(CMAKE_AUTOMOC ON)

# One program per header, as QtTest runs one test object per program.
function(qsubstate_add_test _name)
    add_executable(${_name} ${_name}.cpp QTestTree.h)
    target_link_libraries(${_name} PRIVATE qsubstate Qt6::Test)

    # For TestTree.h and HistoryReference.h, which the tests of substate define.
    target_include_directories(${_name} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../substate)

    set_target_properties(${_name} PROPERTIES
        CXX_EXTENSIONS OFF
        CXX_STANDARD 17
        CXX_STANDARD_REQUIRED ON
    )
    add_test(NAME ${_name} COMMAND ${_name})
endfunction()

qsubstate_add_test(test_Property)
qsubstate_add_test(test_StructNode)
qsubstate_add_test(test_MappingNode)
qsubstate_add_test(test_ModelNotifier)
qsubstate_add_test(test_QCodec)
qsubstate_add_test(test_QModel)
