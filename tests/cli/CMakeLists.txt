project(cli)

file(GLOB _src *.h *.cpp)

add_executable(${PROJECT_NAME} ${_src})
target_link_libraries(${PROJECT_NAME} PRIVATE substate)
target_compile_features(${PROJECT_NAME} PUBLIC cxx_std_17)