project(substate VERSION ${SUBSTATE_VERSION})

substate_add_library(${PROJECT_NAME})

file(GLOB_RECURSE _src *.h *.cpp)

# SheetNode, BytesNode and FilesystemStorageEngine still use the ownership representation that
# docs/Design.md replaces. They rejoin the build in step 3 of the implementation order.
list(FILTER _src EXCLUDE REGEX "/(SheetNode|BytesNode|FilesystemStorageEngine)\\.cpp$")

target_sources(${PROJECT_NAME} PRIVATE ${_src})

target_include_directories(${PROJECT_NAME} PRIVATE
    ${SUBSTATE_SOURCE_DIR}/include/${PROJECT_NAME}/private
)
