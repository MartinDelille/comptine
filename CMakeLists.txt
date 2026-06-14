cmake_minimum_required(VERSION 3.16)

# Extract version from git tags Format: x.y for release, x.y-dev-<sha> for dev
# builds
set(DEFAULT_VERSION "0.0")
set(APP_VERSION "${DEFAULT_VERSION}")
set(APP_VERSION_SUFFIX "")
set(APP_COMMIT_HASH "")

add_definitions(-DPROJECT_ROOT_DIR="${CMAKE_SOURCE_DIR}")

find_package(Git QUIET)
if(GIT_FOUND)
    # Get the nearest tag
    execute_process(
        COMMAND ${GIT_EXECUTABLE} describe --tags --abbrev=0
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        OUTPUT_VARIABLE GIT_TAG
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET
        RESULT_VARIABLE GIT_TAG_RESULT
    )

    # Get short commit hash
    execute_process(
        COMMAND ${GIT_EXECUTABLE} rev-parse --short HEAD
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        OUTPUT_VARIABLE APP_COMMIT_HASH
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET
    )

    if(GIT_TAG_RESULT EQUAL 0 AND GIT_TAG)
        set(APP_VERSION "${GIT_TAG}")

        # Get number of commits since tag
        execute_process(
            COMMAND ${GIT_EXECUTABLE} rev-list ${GIT_TAG}..HEAD --count
            WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
            OUTPUT_VARIABLE COMMITS_SINCE_TAG
            OUTPUT_STRIP_TRAILING_WHITESPACE
            ERROR_QUIET
        )

        # Dev build: add -dev-<sha> suffix if there are commits since tag
        if(COMMITS_SINCE_TAG GREATER 0)
            set(APP_VERSION_SUFFIX "-dev-${APP_COMMIT_HASH}")
        endif()
    else()
        # No tags found, use default with dev suffix
        set(APP_VERSION_SUFFIX "-dev-${APP_COMMIT_HASH}")
    endif()
else()
    # Git not found, use default with dev suffix
    set(APP_VERSION_SUFFIX "-dev")
endif()

set(APP_VERSION_FULL "${APP_VERSION}${APP_VERSION_SUFFIX}")
message(STATUS "Comptine version: ${APP_VERSION_FULL}")

# Parse version for CMake project() - extract major.minor
string(REGEX MATCH "^([0-9]+)\\.([0-9]+)" VERSION_MATCH "${APP_VERSION}")
if(VERSION_MATCH)
    set(VERSION_MAJOR "${CMAKE_MATCH_1}")
    set(VERSION_MINOR "${CMAKE_MATCH_2}")
else()
    set(VERSION_MAJOR "0")
    set(VERSION_MINOR "0")
endif()

project(Comptine VERSION ${VERSION_MAJOR}.${VERSION_MINOR} LANGUAGES CXX)

# Enable Objective-C++ on MacOS (required for FileCoordinator.mm)
if(APPLE)
    enable_language(OBJCXX)
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# On MacOS, skip install rpath fixup since macdeployqt handles Qt library
# deployment This prevents conflicts with duplicate rpath modifications during
# CPack
if(APPLE)
    set(CMAKE_SKIP_INSTALL_RPATH TRUE)
endif()

# Generate Version.h with version information
configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/Version.h.in
    ${CMAKE_CURRENT_BINARY_DIR}/Version.h
    @ONLY
)
include_directories(${CMAKE_CURRENT_BINARY_DIR})

# yaml-cpp (managed by Conan)
find_package(yaml-cpp REQUIRED)

find_package(Qt6 REQUIRED COMPONENTS Quick LinguistTools Network)

qt_standard_project_setup(REQUIRES 6.8)

# libComptine - Core Business Logic Library
# Static library containing all business logic, models, and controllers.
# Declared as a QML module so that classes with QML_ELEMENT are properly
# registered in the QML type system.
# Used by both the main application and unit tests to avoid code duplication.

qt_add_library(libComptine STATIC)

qt_add_qml_module(libComptine
  URI ComptineLib
  OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/ComptineLib
  SOURCES
    AppSettings.cpp AppSettings.h
    Operation.cpp Operation.h
    Account.cpp Account.h
    Category.cpp Category.h
    BudgetData.cpp BudgetData.h
    OperationListModel.cpp OperationListModel.h
    AccountListModel.cpp AccountListModel.h
    RuleListModel.cpp RuleListModel.h
    CategoryController.cpp CategoryController.h
    ClipboardController.cpp ClipboardController.h
    NavigationController.cpp NavigationController.h
    Rule.cpp Rule.h
    RuleController.cpp RuleController.h
    FileController.cpp FileController.h
    TranslationManager.cpp TranslationManager.h
    UpdateController.cpp UpdateController.h
    UndoCommands.cpp UndoCommands.h
    CsvParser.h
    PropertyMacros.h
)

# Link Qt dependencies
target_link_libraries(
    libComptine
    PUBLIC Qt6::Core Qt6::Gui Qt6::Qml Qt6::Network yaml-cpp::yaml-cpp
)

# Include generated Version.h
target_include_directories(libComptine PUBLIC ${CMAKE_CURRENT_BINARY_DIR})

qt_add_executable(Comptine main.cpp)

if(MSVC)
    target_compile_options(Comptine PRIVATE /W4)
    target_compile_options(libComptine PRIVATE /W4)
else()
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -g")
    target_compile_options(
        Comptine
        PRIVATE -Wall -Wextra -Werror -Wswitch-enum -Werror=implicit-fallthrough
    )
    target_compile_options(
        libComptine
        PRIVATE -Wall -Wextra -Werror -Wswitch-enum -Werror=implicit-fallthrough
    )
endif()

# Generate platform-specific icons from SVG at build time
set(APP_ICON_SVG ${CMAKE_CURRENT_SOURCE_DIR}/comptine.svg)
set(APP_ICON_SMALL_SVG
    ${CMAKE_CURRENT_SOURCE_DIR}/docs/assets/comptine-small.svg
)

set(GENERATE_ICON ${CMAKE_CURRENT_SOURCE_DIR}/scripts/generate_icon.py)

if(APPLE)
    set(APP_ICON_ICNS ${CMAKE_CURRENT_BINARY_DIR}/comptine.icns)

    add_custom_command(
        OUTPUT ${APP_ICON_ICNS}
        COMMAND
            uv run python ${GENERATE_ICON} --os macos --svg ${APP_ICON_SVG}
            --svg-small ${APP_ICON_SMALL_SVG} --out ${APP_ICON_ICNS}
        DEPENDS ${APP_ICON_SVG} ${APP_ICON_SMALL_SVG} ${GENERATE_ICON}
        COMMENT "Generating macOS icon from comptine.svg and comptine-small.svg"
    )

    set_source_files_properties(
        ${APP_ICON_ICNS}
        PROPERTIES MACOSX_PACKAGE_LOCATION "Resources"
    )
    target_sources(Comptine PRIVATE ${APP_ICON_ICNS})
elseif(WIN32)
    set(APP_ICON_ICO ${CMAKE_CURRENT_BINARY_DIR}/comptine.ico)
    set(APP_ICON_RC ${CMAKE_CURRENT_BINARY_DIR}/comptine.rc)

    add_custom_command(
        OUTPUT ${APP_ICON_ICO}
        COMMAND
            uv run python ${GENERATE_ICON} --os windows --svg ${APP_ICON_SVG}
            --svg-small ${APP_ICON_SMALL_SVG} --out ${APP_ICON_ICO}
        DEPENDS ${APP_ICON_SVG} ${APP_ICON_SMALL_SVG} ${GENERATE_ICON}
        COMMENT
            "Generating Windows icon from comptine.svg and comptine-small.svg"
    )

    # Generate RC file that references the icon
    file(WRITE ${APP_ICON_RC} "IDI_ICON1 ICON \"comptine.ico\"\n")

    # Add custom target to ensure icon is built before the RC file is compiled
    add_custom_target(WindowsIcon DEPENDS ${APP_ICON_ICO})
    add_dependencies(Comptine WindowsIcon)

    target_sources(Comptine PRIVATE ${APP_ICON_RC})
endif()

add_subdirectory(budget)
add_subdirectory(commonui)
add_subdirectory(operations)
add_subdirectory(rules)

qt_add_qml_module(
  Comptine
  URI
  Comptine
  DEPENDENCIES
  ComptineLib
  Budget
  CommonUI
  Operations
  Rules
  QML_FILES
  Main.qml
  ApplicationMenuBar.qml
  AboutDialog.qml
  ImportDialog.qml
  PreferencesDialog.qml
  UpdateDialog.qml
  SOURCES
  AppState.cpp AppState.h
)

# Qt for iOS sets MACOSX_BUNDLE_GUI_IDENTIFIER automatically since Qt 6.1. If
# you are developing for iOS or MacOS you should consider setting an explicit,
# fixed bundle identifier manually though.
set_target_properties(
    Comptine
    PROPERTIES
        MACOSX_BUNDLE_GUI_IDENTIFIER org.delille.martin.Comptine
        MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
        MACOSX_BUNDLE_SHORT_VERSION_STRING
            ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
        MACOSX_BUNDLE_ICON_FILE comptine.icns
        MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_SOURCE_DIR}/Info.plist.in
        MACOSX_BUNDLE TRUE
        WIN32_EXECUTABLE TRUE
)

# On Linux, rename executable to lowercase to avoid collision with Comptine/ QML
# module directory
if(UNIX AND NOT APPLE)
    set_target_properties(Comptine PROPERTIES OUTPUT_NAME comptine)
endif()

target_link_libraries(
    Comptine
    PRIVATE
        Qt6::Quick
        Qt6::Network
        yaml-cpp::yaml-cpp
        libComptine
        libComptineplugin
        Budget
        CommonUI
        Operations
        Rules
)

# Platform-specific file coordination for cloud storage support (Dropbox,
# iCloud, etc.)
if(APPLE)
    target_sources(libComptine PRIVATE FileCoordinator.h FileCoordinator.mm)
    target_sources(Comptine PRIVATE FileCoordinator.h FileCoordinator.mm)
    target_link_libraries(libComptine PUBLIC "-framework Foundation")
    target_link_libraries(Comptine PRIVATE "-framework Foundation")
else()
    target_sources(
        libComptine
        PRIVATE FileCoordinator.h FileCoordinator_fallback.cpp
    )
    target_sources(
        Comptine
        PRIVATE FileCoordinator.h FileCoordinator_fallback.cpp
    )
endif()

# Translations
qt_add_translations(Comptine TS_FILES translations/comptine_fr.ts
                    RESOURCE_PREFIX "/i18n"
)

include(GNUInstallDirs)
install(
    TARGETS Comptine
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

# Windows: Deploy Qt libraries at install time using
# qt_generate_deploy_qml_app_script
if(WIN32)
    qt_generate_deploy_qml_app_script(TARGET Comptine OUTPUT_SCRIPT deploy_script
                                      NO_UNSUPPORTED_PLATFORM_ERROR
    )
    install(SCRIPT ${deploy_script})
endif()

# Unit tests
enable_testing()
find_package(Qt6 REQUIRED COMPONENTS Test)

# CsvParserTest - header-only library test
qt_add_executable(CsvParserTest tests/CsvParserTest.cpp)
target_link_libraries(CsvParserTest PRIVATE Qt6::Test libComptine)
add_test(NAME CsvParserTest COMMAND CsvParserTest)

# CategoryTest - unit tests for Category class
qt_add_executable(CategoryTest tests/CategoryTest.cpp)
target_link_libraries(CategoryTest PRIVATE Qt6::Test libComptine)
add_test(NAME CategoryTest COMMAND CategoryTest)

# FileControllerTest - integration test with all dependencies
qt_add_executable(FileControllerTest tests/FileControllerTest.cpp FileCoordinator.h FileCoordinator_fallback.cpp)

qt_add_resources(
  FileControllerTest
  FileControllerTestResources
  FILES
  tests/example.comptine
  tests/old.comptine
  tests/import1.csv
  tests/import2.csv
  tests/money.csv
  tests/money2.csv
)

target_link_libraries(FileControllerTest PRIVATE Qt6::Test libComptine)
add_test(NAME FileControllerTest COMMAND FileControllerTest)

# CPack Installer Configuration
set(CPACK_PACKAGE_NAME "Comptine")
set(CPACK_PACKAGE_VENDOR "Martin Delille")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Personal budget management application")
set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
set(CPACK_PACKAGE_VERSION_MAJOR ${PROJECT_VERSION_MAJOR})
set(CPACK_PACKAGE_VERSION_MINOR ${PROJECT_VERSION_MINOR})
set(CPACK_PACKAGE_INSTALL_DIRECTORY "Comptine")

# License file (optional)
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE")
    set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE")
endif()

if(APPLE)
    # MacOS: DragNDrop DMG installer
    set(CPACK_GENERATOR "DragNDrop")
    set(CPACK_DMG_VOLUME_NAME "Comptine")
    set(CPACK_DMG_FORMAT "UDZO")
    set(CPACK_PACKAGE_ICON "${APP_ICON_ICNS}")
    set(CPACK_PACKAGE_FILE_NAME "${CPACK_PACKAGE_NAME}-MacOS")

    # Include the deployed app bundle
    set(CPACK_BUNDLE_NAME "Comptine")
elseif(WIN32)
    # Windows: NSIS installer
    set(CPACK_GENERATOR "NSIS")
    set(CPACK_PACKAGE_FILE_NAME "${CPACK_PACKAGE_NAME}-Windows")

    # Basic NSIS settings
    set(CPACK_NSIS_DISPLAY_NAME "Comptine")
    set(CPACK_NSIS_PACKAGE_NAME "Comptine")
    set(CPACK_NSIS_INSTALLED_ICON_NAME "bin\\\\Comptine.exe")
    set(CPACK_NSIS_MUI_ICON "${CMAKE_CURRENT_BINARY_DIR}/comptine.ico")
    set(CPACK_NSIS_MUI_UNIICON "${CMAKE_CURRENT_BINARY_DIR}/comptine.ico")

    # Option to run application after installation
    set(CPACK_NSIS_MUI_FINISHPAGE_RUN "..\\\\bin\\\\Comptine.exe")

    # Start Menu and Desktop shortcuts
    set(CPACK_NSIS_CREATE_ICONS_EXTRA
        "CreateShortCut '$SMPROGRAMS\\\\$STARTMENU_FOLDER\\\\Comptine.lnk' '$INSTDIR\\\\bin\\\\Comptine.exe'
     CreateShortCut '$DESKTOP\\\\Comptine.lnk' '$INSTDIR\\\\bin\\\\Comptine.exe'"
    )
    set(CPACK_NSIS_DELETE_ICONS_EXTRA
        "Delete '$SMPROGRAMS\\\\$START_MENU\\\\Comptine.lnk'
     Delete '$DESKTOP\\\\Comptine.lnk'"
    )

    # Add/Remove Programs entry
    set(CPACK_NSIS_HELP_LINK "https://github.com/mart1n/Comptine")
    set(CPACK_NSIS_URL_INFO_ABOUT "https://github.com/mart1n/Comptine")
    set(CPACK_NSIS_MODIFY_PATH OFF)
    set(CPACK_NSIS_ENABLE_UNINSTALL_BEFORE_INSTALL ON)

    # File associations for .comptine files
    set(CPACK_NSIS_EXTRA_INSTALL_COMMANDS
        "
    WriteRegStr HKCR '.comptine' '' 'Comptine.Budget'
    WriteRegStr HKCR 'Comptine.Budget' '' 'Comptine Budget File'
    WriteRegStr HKCR 'Comptine.Budget\\\\DefaultIcon' '' '$INSTDIR\\\\bin\\\\Comptine.exe,0'
    WriteRegStr HKCR 'Comptine.Budget\\\\shell\\\\open\\\\command' '' '$\\\"$INSTDIR\\\\bin\\\\Comptine.exe$\\\" $\\\"%1$\\\"'
    System::Call 'Shell32::SHChangeNotify(i 0x08000000, i 0, p 0, p 0)'
  "
    )
    set(CPACK_NSIS_EXTRA_UNINSTALL_COMMANDS
        "
    DeleteRegKey HKCR '.comptine'
    DeleteRegKey HKCR 'Comptine.Budget'
    System::Call 'Shell32::SHChangeNotify(i 0x08000000, i 0, p 0, p 0)'
  "
    )
endif()

include(CPack)
