cmake_minimum_required(VERSION 3.19)

project(untitled19 LANGUAGES CXX)

# Установка стандарта C++17
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Поиск необходимых компонентов Qt
find_package(Qt6 6.5 REQUIRED COMPONENTS Core Widgets Sql LinguistTools)

# Установка политик CMake
if(POLICY CMP0071)
    cmake_policy(SET CMP0071 NEW)
endif()

# Настройка стандартного проекта Qt
qt_standard_project_setup()

# Список исходных файлов
set(SOURCES
    main.cpp
    mainwindow.cpp
    mainwindow.h
    mainwindow.ui
)

# Создание исполняемого файла
qt_add_executable(untitled19
    ${SOURCES}
)

# Настройка свойств приложения
set_target_properties(untitled19 PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE
)

# Добавление переводов (если используются)
qt_add_translations(untitled19
    TS_FILES untitled19_en_US.ts
    QM_FILES_OUTPUT_VARIABLE translation_qm_files
)

# Подключение необходимых библиотек
target_link_libraries(untitled19 PRIVATE
    Qt6::Core
    Qt6::Widgets
    Qt6::Sql
)

# Поиск SQLite3 (если требуется прямое использование)
find_package(SQLite3)
if(SQLite3_FOUND)
    target_link_libraries(untitled19 PRIVATE SQLite::SQLite3)
endif()

# Установка приложения
include(GNUInstallDirs)
install(TARGETS untitled19
    BUNDLE  DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

# Генерация скрипта для развертывания
qt_generate_deploy_app_script(
    TARGET untitled19
    OUTPUT_SCRIPT deploy_script
    NO_UNSUPPORTED_PLATFORM_ERROR
)
install(SCRIPT ${deploy_script})

# Добавление ресурсов, если есть
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/resources.qrc)
    qt_add_resources(untitled19 "resources"
        PREFIX "/"
        FILES resources.qrc
    )
endif()
