cmake_minimum_required(VERSION 3.19)

project(Task_Manager_dev LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 6.5 REQUIRED COMPONENTS Core Widgets Sql LinguistTools)

if(POLICY CMP0071)
    cmake_policy(SET CMP0071 NEW)
endif()

qt_standard_project_setup()

set(SOURCES
    main.cpp
    mainwindow.cpp
    mainwindow.h
    mainwindow.ui
)

qt_add_executable(Task_Manager_dev
    ${SOURCES}
)

set_target_properties(Task_Manager_dev PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE
)

target_link_libraries(Task_Manager_dev PRIVATE
    Qt6::Core
    Qt6::Widgets
    Qt6::Sql
)

find_package(SQLite3)
if(SQLite3_FOUND)
    target_link_libraries(Task_Manager_dev PRIVATE SQLite::SQLite3)
endif()

include(GNUInstallDirs)
install(TARGETS Task_Manager_dev
    BUNDLE  DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

qt_generate_deploy_app_script(
    TARGET Task_Manager_dev
    OUTPUT_SCRIPT deploy_script
    NO_UNSUPPORTED_PLATFORM_ERROR
)
install(SCRIPT ${deploy_script})

if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/resources.qrc)
    qt_add_resources(Task_Manager_dev "resources"
        PREFIX "/"
        FILES resources.qrc
    )
endif()
