project(tst_variant)

# Set language options
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED on)

file(GLOB _src *.h *.cpp)

add_executable(${PROJECT_NAME} ${_src})

target_link_libraries(${PROJECT_NAME} PRIVATE substate)