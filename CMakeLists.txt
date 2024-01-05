cmake_minimum_required(VERSION 3.17)

project(substate VERSION 0.0.1.1 LANGUAGES CXX)

# ----------------------------------
# Build Options
# ----------------------------------
option(SUBSTATE_BUILD_STATIC "Build static libraries" OFF)
option(SUBSTATE_BUILD_TRANSLATIONS "Build translations" ON)
option(SUBSTATE_BUILD_TESTS "Build test cases" ON)
option(SUBSTATE_BUILD_QT_TARGETS "Build targets related to Qt" ON)
option(SUBSTATE_BUILD_DOCUMENTATIONS "Build documentations" OFF)
option(SUBSTATE_INSTALL "Install library" ON)

# ----------------------------------
# CMake Settings
# ----------------------------------
if(MSVC)
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /manifest:no")
    set(CMAKE_MODULE_LINKER_FLAGS "${CMAKE_MODULE_LINKER_FLAGS} /manifest:no")
    set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} /manifest:no")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /utf-8")

    if(NOT DEFINED CMAKE_DEBUG_POSTFIX)
        set(CMAKE_DEBUG_POSTFIX "d")
    endif()
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

# ----------------------------------
# Main Project
# ----------------------------------
find_package(qmsetup REQUIRED)

add_subdirectory(src)

if(SUBSTATE_BUILD_TESTS)
    add_subdirectory(tests)
endif()