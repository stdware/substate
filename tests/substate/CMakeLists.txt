project(test_substate LANGUAGES CXX)

find_package(Boost CONFIG COMPONENTS unit_test_framework)

# Fatal rather than a warning and a return, because a successful build without tests after
# SUBSTATE_BUILD_TESTS=ON would report nothing.
if(NOT Boost_FOUND)
    message(FATAL_ERROR
        "SUBSTATE_BUILD_TESTS is on but Boost.Test was not found. Install it, or point CMake at "
        "it with -DCMAKE_PREFIX_PATH, or turn the tests off with -DSUBSTATE_BUILD_TESTS=OFF.")
endif()

file(GLOB _src *.h *.cpp)
add_executable(${PROJECT_NAME} ${_src})

target_link_libraries(${PROJECT_NAME} PRIVATE Boost::unit_test_framework substate)

set_target_properties(${PROJECT_NAME} PROPERTIES
    CXX_EXTENSIONS OFF
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
)

add_test(NAME ${PROJECT_NAME} COMMAND ${PROJECT_NAME})
