qt_add_executable(Comptine main.cpp)

target_compile_definitions(
    Comptine
    PRIVATE PROJECT_ROOT_DIR="${PROJECT_SOURCE_DIR}"
)

qt_add_qml_module(Comptine
    URI app
    # The application module contains no C++ QML types of its own.
    NO_GENERATE_QMLTYPES
    IMPORTS
        TARGET common
        TARGET rules
        TARGET budget
        TARGET operations
        TARGET services
        TARGET editor
        TARGET model
    QML_FILES
        Main.qml
        ApplicationMenuBar.qml
        FileMenu.qml
        EditMenu.qml
        ViewMenu.qml
        HelpMenu.qml
        AboutDialog.qml
        ImportDialog.qml
        PreferencesDialog.qml
        UpdateDialog.qml
)

if(APPLE)
    include(cmake/macos.cmake)
elseif(WIN32)
    include(cmake/windows.cmake)
endif()

target_link_libraries(
    Comptine
    PRIVATE
        Qt6::Quick
        Qt6::Network
        yaml-cpp::yaml-cpp
        project_warnings
        services
        editor
        budget
        common
        operations
        rules
)

qt_add_translations(Comptine TS_FILES ${PROJECT_SOURCE_DIR}/translations/comptine_fr.ts
                    RESOURCE_PREFIX "/i18n"
)

install(
    TARGETS Comptine
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
