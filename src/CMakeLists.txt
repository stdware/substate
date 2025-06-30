function(substate_add_library _target)
    string(TOUPPER ${_target} _target_upper)

    if(SUBSTATE_BUILD_STATIC OR(NOT SUBSTATE_BUILD_SHARED AND NOT BUILD_SHARED_LIBS))
        add_library(${_target} STATIC)
        target_compile_definitions(${_target} PUBLIC ${_target_upper}_STATIC)
    else()
        add_library(${_target} SHARED)
    endif()

    add_library(${SUBSTATE_INSTALL_NAME}::${_target} ALIAS ${_target})

    target_compile_definitions(${_target} PRIVATE ${_target_upper}_LIBRARY)
    target_include_directories(${_target} INTERFACE
        $<BUILD_INTERFACE:${SUBSTATE_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
    )
    target_include_directories(${_target} PRIVATE
        ${SUBSTATE_SOURCE_DIR}/include
        ${SUBSTATE_SOURCE_DIR}/include/${_target}
        src/${_target}
    )

    target_compile_features(${_target} PUBLIC cxx_std_17)
    set_target_properties(${_target} PROPERTIES
        CXX_STANDARD 17
        CXX_STANDARD_REQUIRED ON
    )

    # Add platform specific
    if(WIN32)
        set(RC_DESCRIPTION "Crash Consistent Undo Framework")
        set(RC_COPYRIGHT "Copyright (C) 2022-2025 Stdware Collections")
        include("${SUBSTATE_SOURCE_DIR}/cmake/winrc.cmake")
    endif()

    if(SUBSTATE_INSTALL)
        install(TARGETS ${_target}
            EXPORT ${SUBSTATE_INSTALL_NAME}Targets
            RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}" OPTIONAL
            LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}" OPTIONAL
            ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}" OPTIONAL
            COMPONENT ${_target}
        )
    endif()

    if(NOT WIN32)
        target_link_libraries(${_target} PRIVATE dl)
        target_link_options(${_target} PRIVATE -Wl,-z,defs)
    endif()
endfunction()

add_subdirectory(substate)

if(SUBSTATE_INSTALL)
    # Add install target
    set(_install_dir ${CMAKE_INSTALL_LIBDIR}/cmake/${SUBSTATE_INSTALL_NAME})

    # Add version file
    write_basic_package_version_file(
        "${CMAKE_CURRENT_BINARY_DIR}/${SUBSTATE_INSTALL_NAME}ConfigVersion.cmake"
        VERSION ${PROJECT_VERSION}
        COMPATIBILITY AnyNewerVersion
    )

    # Add configuration file
    configure_package_config_file(
        ${CMAKE_CURRENT_LIST_DIR}/${SUBSTATE_INSTALL_NAME}Config.cmake.in
        "${CMAKE_CURRENT_BINARY_DIR}/${SUBSTATE_INSTALL_NAME}Config.cmake"
        INSTALL_DESTINATION ${_install_dir}
        NO_CHECK_REQUIRED_COMPONENTS_MACRO
    )

    # Install cmake files
    install(FILES
        "${CMAKE_CURRENT_BINARY_DIR}/${SUBSTATE_INSTALL_NAME}Config.cmake"
        "${CMAKE_CURRENT_BINARY_DIR}/${SUBSTATE_INSTALL_NAME}ConfigVersion.cmake"
        DESTINATION ${_install_dir}
    )

    # Install cmake targets files
    install(EXPORT ${SUBSTATE_INSTALL_NAME}Targets
        FILE "${SUBSTATE_INSTALL_NAME}Targets.cmake"
        NAMESPACE substate::
        DESTINATION ${_install_dir}
    )

    install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/../include
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
        FILES_MATCHING PATTERN "*.h"
    )
endif()