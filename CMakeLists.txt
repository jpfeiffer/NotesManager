cmake_minimum_required(VERSION 3.5)

project(NotesManager VERSION 0.1 LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets LinguistTools)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Concurrent Widgets LinguistTools)

set(TS_FILES NotesManager_de_DE.ts)
set(QUDEV_LIBRARY QUdev)

set(PROJECT_SOURCES
        main.cpp
        NotesManager.cpp
        NotesManager.h
        NotesManager.ui
        TopicWidget.cpp
        TopicWidget.h
        TopicWidget.ui
        NotesManager.qrc
        ${TS_FILES}
)

add_custom_target(translations)
qt_add_lupdate(translations TS_FILES ${TS_FILES} SOURCES ${PROJECT_SOURCES})
qt_add_lrelease(translations TS_FILES ${TS_FILES} QM_FILES_OUTPUT_VARIABLE qm_files)
add_dependencies(translations_lrelease translations_lupdate)

qt_add_executable(NotesManager
    MANUAL_FINALIZATION
    ${PROJECT_SOURCES}
)

add_dependencies(NotesManager translations)

qt_add_resources(NotesManager "translations"
    PREFIX "/i18n"
    BASE "${CMAKE_CURRENT_BINARY_DIR}"
    FILES "${qm_files}"
)

target_link_libraries(NotesManager PRIVATE Qt${QT_VERSION_MAJOR}::Widgets)
target_link_libraries(NotesManager PRIVATE Qt${QT_VERSION_MAJOR}::Concurrent)
target_link_libraries(NotesManager PRIVATE ${QUDEV_LIBRARY})

set_target_properties(NotesManager PROPERTIES
    MACOSX_BUNDLE_GUI_IDENTIFIER my.example.com
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

install(TARGETS NotesManager
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})

if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(NotesManager)
endif()
