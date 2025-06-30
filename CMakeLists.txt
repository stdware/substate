cmake_minimum_required(VERSION 3.16)

project(substate VERSION 0.0.1.2 LANGUAGES CXX)

# ----------------------------------
# Build Options
# ----------------------------------
option(SUBSTATE_BUILD_STATIC "Build static library" OFF)
option(SUBSTATE_BUILD_SHARED "Build shared library" OFF)
option(SUBSTATE_BUILD_TESTS "Build test cases" OFF)
option(SUBSTATE_BUILD_EXAMPLES "Build examples" OFF)
option(SUBSTATE_INSTALL "Install library" ON)

# ----------------------------------
# CMake Settings
# ----------------------------------
if(NOT DEFINED CMAKE_RUNTIME_OUTPUT_DIRECTORY)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/bin)
endif()

if(NOT DEFINED CMAKE_LIBRARY_OUTPUT_DIRECTORY)
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/lib)
endif()

if(NOT DEFINED CMAKE_ARCHIVE_OUTPUT_DIRECTORY)
    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/lib)
endif()

if(MSVC)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /utf-8")

    if(NOT DEFINED CMAKE_DEBUG_POSTFIX)
        set(CMAKE_DEBUG_POSTFIX "d")
    endif()
elseif(MINGW)
    set(CMAKE_STATIC_LIBRARY_PREFIX "")
    set(CMAKE_SHARED_LIBRARY_PREFIX "")
endif()

if(SUBSTATE_INSTALL)
    include(GNUInstallDirs)
    include(CMakePackageConfigHelpers)
endif()

# ----------------------------------
# Project Variables
# ----------------------------------
set(SUBSTATE_VERSION ${PROJECT_VERSION})
set(SUBSTATE_INSTALL_NAME ${PROJECT_NAME})
set(SUBSTATE_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR})

# ----------------------------------
# Main Project
# ----------------------------------
add_subdirectory(src)

if(SUBSTATE_BUILD_TESTS)
    add_subdirectory(tests)
endif()

if(SUBSTATE_BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()