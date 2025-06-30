project(substate VERSION ${SUBSTATE_VERSION})

substate_add_library(${PROJECT_NAME})

file(GLOB_RECURSE _src *.h *.cpp)
target_sources(${PROJECT_NAME} PRIVATE ${_src})

target_include_directories(${PROJECT_NAME} PRIVATE
    ${SUBSTATE_SOURCE_DIR}/include/${PROJECT_NAME}/private
)