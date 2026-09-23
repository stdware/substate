project(qsubstate VERSION ${SUBSTATE_VERSION})

# Qt 6 only for now. Support for Qt 5 is added later, see docs/Design.md.
find_package(Qt6 COMPONENTS Core REQUIRED)

substate_add_library(${PROJECT_NAME})

# The public headers are listed as sources, because AUTOMOC processes only the sources and
# ModelNotifier.h declares a QObject.
file(GLOB_RECURSE _src *.h *.cpp)
file(GLOB_RECURSE _hdr ${SUBSTATE_SOURCE_DIR}/include/${PROJECT_NAME}/*.h)
target_sources(${PROJECT_NAME} PRIVATE ${_src} ${_hdr})
set_target_properties(${PROJECT_NAME} PROPERTIES AUTOMOC ON)

target_include_directories(${PROJECT_NAME} PRIVATE
    ${SUBSTATE_SOURCE_DIR}/include/${PROJECT_NAME}/private
)

target_link_libraries(${PROJECT_NAME} PUBLIC
    Qt6::Core
    substate
)
