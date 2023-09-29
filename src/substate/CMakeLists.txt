set(_generate_include_dir "${CMAKE_CURRENT_BINARY_DIR}/include")
file(GLOB_RECURSE _src *.h *.cpp)

set(_cxx_flags)

if(MSVC)
    list(APPEND _cxx_flags "/Zc:preprocessor")
endif()

# Set language options
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED on)

# Add library
if(SUBSTATE_BUILD_STATIC)
    add_library(${PROJECT_NAME} STATIC)
else()
    add_library(${PROJECT_NAME} SHARED)

    if(WIN32)
        qtmediate_add_win_rc(${PROJECT_NAME}
            DESCRIPTION "${SUBSTATE_PROJECT_DESCRIPTION}"
            COPYRIGHT "${SUBSTATE_PROJECT_COPYRIGHT}"
        )
    endif()
endif()

# Configure target
add_library(substate::${PROJECT_NAME} ALIAS ${PROJECT_NAME})
qtmediate_configure_target(${PROJECT_NAME}
    SOURCES ${_src}
    INCLUDE_PRIVATE .
    CCFLAGS ${_cxx_flags}
)

# Set global definitions
qtmediate_export_defines(${PROJECT_NAME})

if(SUBSTATE_INSTALL)
    install(TARGETS ${PROJECT_NAME}
        EXPORT ${SUBSTATE_INSTALL_NAME}Targets
        RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}" OPTIONAL
        LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}" OPTIONAL
        ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}" OPTIONAL
    )

    target_include_directories(${PROJECT_NAME} PUBLIC
        "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/${SUBSTATE_INSTALL_NAME}>"
    )

    set(_gen_include_options INSTALL_DIR "${CMAKE_INSTALL_INCLUDEDIR}/${SUBSTATE_INSTALL_NAME}")
else()
    set(_gen_include_options)
endif()

# Generate a standard include directory in build directory
qtmediate_gen_include(. "${_generate_include_dir}/${PROJECT_NAME}" CLEAN ${_gen_include_options})
target_include_directories(${PROJECT_NAME} PUBLIC
    "$<BUILD_INTERFACE:${_generate_include_dir}>"
)