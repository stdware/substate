project(qormc VERSION ${SUBSTATE_VERSION} LANGUAGES CXX)

file(GLOB_RECURSE _src *.h *.cpp)

add_executable(${PROJECT_NAME})

qtmediate_configure_target(${PROJECT_NAME}
    SOURCES ${_src}
    QT_LINKS Core
    DEFINES APP_VERSION=\"${PROJECT_VERSION}\"
)

if(WIN32)
    qtmediate_add_win_rc(${PROJECT_NAME}
        DESCRIPTION "Qt Substate Classes Generator"
        COPYRIGHT "${SUBSTATE_PROJECT_COPYRIGHT}"
    )
endif()