if(NOT DEFINED QTMEDIATE_CMAKE_MODULES_DIR)
    message(FATAL_ERROR "QTMEDIATE_CMAKE_MODULES_DIR not defined!")
endif()

include("${QTMEDIATE_CMAKE_MODULES_DIR}/QtMediateAPI.cmake")

set(SUBSTATE_PROJECT_DESCRIPTION "Crash Consistent Undo Framework")
set(SUBSTATE_PROJECT_COPYRIGHT "Copyright 2019-2023 SineStriker")

# ----------------------------------
# Main Project
# ----------------------------------
add_subdirectory(substate)

add_subdirectory(sqliteengine)

if(SUBSTATE_BUILD_QT_TARGETS)
    add_subdirectory(substate-qt)
endif()

# ----------------------------------
# Documentation
# ----------------------------------
if(SUBSTATE_BUILD_DOCUMENTATIONS)
    if(NOT DOXYGEN_EXECUTABLE)
        find_package(Doxygen REQUIRED)
    endif()

    set(_install_options)

    if(SUBSTATE_INSTALL)
        set(_install_options INSTALL_DIR share/doc/${SUBSTATE_INSTALL_NAME})
    endif()

    set(_doc_targets substate)

    set(SUBSTATE_DOXYGEN_TAGFILE ${CMAKE_BUILD_SHARE_DIR}/doc/${SUBSTATE_INSTALL_NAME}/substate_tagfile.xml
        CACHE FILEPATH "substate doxygen tag file path" FORCE
    )

    qtmediate_setup_doxygen(substate_RunDoxygen
        NAME "Substate"
        DESCRIPTION "${SUBSTATE_PROJECT_DESCRIPTION}"
        MDFILE ../README.md
        OUTPUT_DIR ${CMAKE_BUILD_SHARE_DIR}/doc/${SUBSTATE_INSTALL_NAME}
        INPUT substate
        TARGETS ${_doc_targets}
        DEPENDS ${_doc_targets}
        NO_EXPAND_MACROS Q_OBJECT Q_GADGET Q_DECLARE_TR_FUNCTIONS
        COMPILE_DEFINITIONS Q_SIGNALS=Q_SIGNALS Q_SLOTS=Q_SLOTS
        GENERATE_TAGFILE "${SUBSTATE_DOXYGEN_TAGFILE}"
        ${_install_options}
    )
endif()

# ----------------------------------
# Install
# ----------------------------------
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

    # Install cmake modules
    install(DIRECTORY ${SUBSTATE_MODULES_DIR}/
        DESTINATION ${_install_dir}/cmake
        PATTERN "__pycache__" EXCLUDE
    )
endif()