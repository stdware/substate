project(sqliteengine VERSION ${SUBSTATE_VERSION} LANGUAGES C CXX)

file(GLOB_RECURSE _src *.h *.c *.cpp)

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
        qm_add_win_rc(${PROJECT_NAME}
            DESCRIPTION "${SUBSTATE_PROJECT_DESCRIPTION}"
            COPYRIGHT "${SUBSTATE_PROJECT_COPYRIGHT}"
        )
    endif()
endif()

# Configure target
add_library(substate::${PROJECT_NAME} ALIAS ${PROJECT_NAME})
qm_configure_target(${PROJECT_NAME}
    SOURCES ${_src}
    INCLUDE_PRIVATE .
    CCFLAGS ${_cxx_flags}
    LINKS substate
)

# Set global definitions
qm_export_defines(${PROJECT_NAME})

if(SUBSTATE_INSTALL)
    install(TARGETS ${PROJECT_NAME}
        EXPORT ${SUBSTATE_INSTALL_NAME}Targets
        RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}" OPTIONAL
        LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}" OPTIONAL
        ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}" OPTIONAL
    )

    install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/${SUBSTATE_INSTALL_NAME}/${PROJECT_NAME}

        FILES_MATCHING
        PATTERN "*.h"
        PATTERN "sqlite3*" EXCLUDE
    )

    target_include_directories(${PROJECT_NAME} PUBLIC
        "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/${SUBSTATE_INSTALL_NAME}>"
    )
endif()

target_include_directories(${PROJECT_NAME} PUBLIC
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>"
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/..>"
)