project(qsubstate VERSION ${SUBSTATE_VERSION})

# Qt 6 only for now. Support for Qt 5 is added later, see docs/Design.md.
find_package(Qt6 COMPONENTS Core REQUIRED)

substate_add_library(${PROJECT_NAME})

file(GLOB_RECURSE _src *.h *.cpp)
target_sources(${PROJECT_NAME} PRIVATE ${_src})

target_include_directories(${PROJECT_NAME} PRIVATE
    ${SUBSTATE_SOURCE_DIR}/include/${PROJECT_NAME}/private
)

target_link_libraries(${PROJECT_NAME} PUBLIC
    Qt6::Core
    substate
)
