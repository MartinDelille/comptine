cmake_minimum_required(VERSION 3.16)

# Extract version from git tags
# Format: x.y for release, x.y-dev-<sha> for dev builds
set(DEFAULT_VERSION "0.0")
set(APP_VERSION "${DEFAULT_VERSION}")
set(APP_VERSION_SUFFIX "")
set(APP_COMMIT_HASH "")

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

project(
  Comptine
  VERSION ${VERSION_MAJOR}.${VERSION_MINOR}
  LANGUAGES CXX)

set(CMAKE_CXX_STANDARD_REQUIRED ON)

# On macOS, skip install rpath fixup since macdeployqt handles Qt library deployment
# This prevents conflicts with duplicate rpath modifications during CPack
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

# Fetch rapidyaml (static library, no install)
include(FetchContent)
set(RYML_INSTALL OFF CACHE BOOL "" FORCE)
FetchContent_Declare(
  rapidyaml
  GIT_REPOSITORY https://github.com/biojppm/rapidyaml.git
  GIT_TAG v0.7.2
  GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(rapidyaml)

find_package(Qt6 REQUIRED COMPONENTS Quick LinguistTools Network)

qt_standard_project_setup(REQUIRES 6.8)

qt_add_executable(Comptine
  main.cpp
  PropertyMacros.h
  UndoCommands.h
  UndoCommands.cpp
)

# Generate platform-specific icons from SVG at build time
set(APP_ICON_SVG ${CMAKE_CURRENT_SOURCE_DIR}/comptine.svg)
set(APP_ICON_SMALL_SVG ${CMAKE_CURRENT_SOURCE_DIR}/comptine-small.svg)

if(APPLE)
  set(APP_ICON_PNG ${CMAKE_CURRENT_BINARY_DIR}/comptine.png)
  set(APP_ICON_SMALL_PNG ${CMAKE_CURRENT_BINARY_DIR}/comptine-small.png)
  set(APP_ICON_ICNS ${CMAKE_CURRENT_BINARY_DIR}/comptine.icns)
  set(APP_ICONSET_DIR ${CMAKE_CURRENT_BINARY_DIR}/comptine.iconset)

  # Use ImageMagick to render SVGs to PNG with transparency preserved
  # Use small version for icons < 128px, full version for >= 128px
  add_custom_command(
    OUTPUT ${APP_ICON_ICNS}
    COMMAND magick -background none ${APP_ICON_SVG} -resize 1024x1024 ${APP_ICON_PNG}
    COMMAND magick -background none ${APP_ICON_SMALL_SVG} -resize 64x64 ${APP_ICON_SMALL_PNG}
    COMMAND ${CMAKE_COMMAND} -E make_directory ${APP_ICONSET_DIR}
    COMMAND sips -z 16 16 ${APP_ICON_SMALL_PNG} --out ${APP_ICONSET_DIR}/icon_16x16.png
    COMMAND sips -z 32 32 ${APP_ICON_SMALL_PNG} --out ${APP_ICONSET_DIR}/icon_16x16@2x.png
    COMMAND sips -z 32 32 ${APP_ICON_SMALL_PNG} --out ${APP_ICONSET_DIR}/icon_32x32.png
    COMMAND sips -z 64 64 ${APP_ICON_SMALL_PNG} --out ${APP_ICONSET_DIR}/icon_32x32@2x.png
    COMMAND sips -z 128 128 ${APP_ICON_PNG} --out ${APP_ICONSET_DIR}/icon_128x128.png
    COMMAND sips -z 256 256 ${APP_ICON_PNG} --out ${APP_ICONSET_DIR}/icon_128x128@2x.png
    COMMAND sips -z 256 256 ${APP_ICON_PNG} --out ${APP_ICONSET_DIR}/icon_256x256.png
    COMMAND sips -z 512 512 ${APP_ICON_PNG} --out ${APP_ICONSET_DIR}/icon_256x256@2x.png
    COMMAND sips -z 512 512 ${APP_ICON_PNG} --out ${APP_ICONSET_DIR}/icon_512x512.png
    COMMAND sips -z 1024 1024 ${APP_ICON_PNG} --out ${APP_ICONSET_DIR}/icon_512x512@2x.png
    COMMAND iconutil -c icns ${APP_ICONSET_DIR} -o ${APP_ICON_ICNS}
    DEPENDS ${APP_ICON_SVG} ${APP_ICON_SMALL_SVG}
    COMMENT "Generating macOS icon from comptine.svg and comptine-small.svg"
  )

  set_source_files_properties(${APP_ICON_ICNS} PROPERTIES MACOSX_PACKAGE_LOCATION "Resources")
  target_sources(Comptine PRIVATE ${APP_ICON_ICNS})
elseif(WIN32)
  set(APP_ICON_ICO ${CMAKE_CURRENT_BINARY_DIR}/comptine.ico)
  set(APP_ICON_RC ${CMAKE_CURRENT_BINARY_DIR}/comptine.rc)
  set(APP_ICON_PNG ${CMAKE_CURRENT_BINARY_DIR}/comptine.png)
  set(APP_ICON_SMALL_PNG ${CMAKE_CURRENT_BINARY_DIR}/comptine-small.png)

  # Use ImageMagick to create multi-resolution ICO file
  # Use small version for icons < 128px, full version for >= 128px
  add_custom_command(
    OUTPUT ${APP_ICON_ICO}
    COMMAND magick -background none ${APP_ICON_SVG} -resize 256x256 ${APP_ICON_PNG}
    COMMAND magick -background none ${APP_ICON_SMALL_SVG} -resize 64x64 ${APP_ICON_SMALL_PNG}
    COMMAND magick ${APP_ICON_SMALL_PNG} -resize 16x16 ${CMAKE_CURRENT_BINARY_DIR}/icon_16.png
    COMMAND magick ${APP_ICON_SMALL_PNG} -resize 32x32 ${CMAKE_CURRENT_BINARY_DIR}/icon_32.png
    COMMAND magick ${APP_ICON_SMALL_PNG} -resize 48x48 ${CMAKE_CURRENT_BINARY_DIR}/icon_48.png
    COMMAND magick ${APP_ICON_SMALL_PNG} -resize 64x64 ${CMAKE_CURRENT_BINARY_DIR}/icon_64.png
    COMMAND magick ${APP_ICON_PNG} -resize 128x128 ${CMAKE_CURRENT_BINARY_DIR}/icon_128.png
    COMMAND magick ${APP_ICON_PNG} -resize 256x256 ${CMAKE_CURRENT_BINARY_DIR}/icon_256.png
    COMMAND magick ${CMAKE_CURRENT_BINARY_DIR}/icon_16.png ${CMAKE_CURRENT_BINARY_DIR}/icon_32.png ${CMAKE_CURRENT_BINARY_DIR}/icon_48.png ${CMAKE_CURRENT_BINARY_DIR}/icon_64.png ${CMAKE_CURRENT_BINARY_DIR}/icon_128.png ${CMAKE_CURRENT_BINARY_DIR}/icon_256.png ${APP_ICON_ICO}
    DEPENDS ${APP_ICON_SVG} ${APP_ICON_SMALL_SVG}
    COMMENT "Generating Windows icon from comptine.svg and comptine-small.svg"
  )

  # Generate RC file that references the icon
  file(WRITE ${APP_ICON_RC} "IDI_ICON1 ICON \"comptine.ico\"\n")

  # Add custom target to ensure icon is built before the RC file is compiled
  add_custom_target(WindowsIcon DEPENDS ${APP_ICON_ICO})
  add_dependencies(Comptine WindowsIcon)

  target_sources(Comptine PRIVATE ${APP_ICON_RC})
endif()

set_source_files_properties(Theme.qml PROPERTIES QT_QML_SINGLETON_TYPE TRUE)

qt_add_qml_module(Comptine URI Comptine
    QML_FILES
        Main.qml
        AboutDialog.qml
        AmountField.qml
        BalanceHeader.qml
        BudgetView.qml
        CategoryDetailView.qml
        CategoryEditDialog.qml
        ImportDialog.qml
        MonthSelector.qml
        OperationDelegate.qml
        OperationDetails.qml
        OperationEditDialog.qml
        OperationList.qml
        OperationView.qml
        PreferencesDialog.qml
        Theme.qml
        UpdateDialog.qml
    SOURCES
        Operation.h Operation.cpp
        Account.h Account.cpp
        Category.h Category.cpp
        BudgetData.h BudgetData.cpp
        OperationListModel.h OperationListModel.cpp
        AccountListModel.h AccountListModel.cpp
        AppSettings.h AppSettings.cpp
        AppState.h AppState.cpp
        CategoryController.h CategoryController.cpp
        ClipboardController.h ClipboardController.cpp
        NavigationController.h NavigationController.cpp
        FileController.h FileController.cpp
        TranslationManager.h TranslationManager.cpp
        UpdateController.h UpdateController.cpp
)

# Qt for iOS sets MACOSX_BUNDLE_GUI_IDENTIFIER automatically since Qt 6.1. If
# you are developing for iOS or macOS you should consider setting an explicit,
# fixed bundle identifier manually though.
set_target_properties(
  Comptine
  PROPERTIES MACOSX_BUNDLE_GUI_IDENTIFIER org.delille.martin.Comptine
             MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
             MACOSX_BUNDLE_SHORT_VERSION_STRING
             ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
             MACOSX_BUNDLE_ICON_FILE comptine.icns
             MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_SOURCE_DIR}/Info.plist.in
             MACOSX_BUNDLE TRUE
             WIN32_EXECUTABLE TRUE)

target_link_libraries(Comptine PRIVATE Qt6::Quick Qt6::Network ryml::ryml)

# Translations
qt_add_translations(Comptine
    TS_FILES
        translations/comptine_fr.ts
    RESOURCE_PREFIX "/i18n"
)

include(GNUInstallDirs)
install(
  TARGETS Comptine
  BUNDLE DESTINATION .
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

# Windows: Deploy Qt libraries at install time using qt_generate_deploy_qml_app_script
if(WIN32)
  qt_generate_deploy_qml_app_script(
    TARGET Comptine
    OUTPUT_SCRIPT deploy_script
    NO_UNSUPPORTED_PLATFORM_ERROR
  )
  install(SCRIPT ${deploy_script})
endif()

# Unit tests
enable_testing()
find_package(Qt6 REQUIRED COMPONENTS Test)
qt_add_executable(CsvParserTest
  tests/CsvParserTest.cpp
  CsvParser.h
)
target_link_libraries(CsvParserTest PRIVATE Qt6::Core Qt6::Test)
add_test(NAME CsvParserTest COMMAND CsvParserTest)

# =============================================================================
# CPack Installer Configuration
# =============================================================================
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
  # macOS: DragNDrop DMG installer
  set(CPACK_GENERATOR "DragNDrop")
  set(CPACK_DMG_VOLUME_NAME "Comptine")
  set(CPACK_DMG_FORMAT "UDZO")
  set(CPACK_PACKAGE_ICON "${APP_ICON_ICNS}")
  set(CPACK_PACKAGE_FILE_NAME "${CPACK_PACKAGE_NAME}-${PROJECT_VERSION}-MacOS")

  # Include the deployed app bundle
  set(CPACK_BUNDLE_NAME "Comptine")

elseif(WIN32)
  # Windows: NSIS installer
  set(CPACK_GENERATOR "NSIS")
  set(CPACK_PACKAGE_FILE_NAME "${CPACK_PACKAGE_NAME}-${PROJECT_VERSION}-Windows")

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
     CreateShortCut '$DESKTOP\\\\Comptine.lnk' '$INSTDIR\\\\bin\\\\Comptine.exe'")
  set(CPACK_NSIS_DELETE_ICONS_EXTRA
    "Delete '$SMPROGRAMS\\\\$START_MENU\\\\Comptine.lnk'
     Delete '$DESKTOP\\\\Comptine.lnk'")

  # Add/Remove Programs entry
  set(CPACK_NSIS_HELP_LINK "https://github.com/mart1n/Comptine")
  set(CPACK_NSIS_URL_INFO_ABOUT "https://github.com/mart1n/Comptine")
  set(CPACK_NSIS_MODIFY_PATH OFF)
  set(CPACK_NSIS_ENABLE_UNINSTALL_BEFORE_INSTALL ON)

  # File associations for .comptine files
  set(CPACK_NSIS_EXTRA_INSTALL_COMMANDS "
    WriteRegStr HKCR '.comptine' '' 'Comptine.Budget'
    WriteRegStr HKCR 'Comptine.Budget' '' 'Comptine Budget File'
    WriteRegStr HKCR 'Comptine.Budget\\\\DefaultIcon' '' '$INSTDIR\\\\bin\\\\Comptine.exe,0'
    WriteRegStr HKCR 'Comptine.Budget\\\\shell\\\\open\\\\command' '' '$\\\"$INSTDIR\\\\bin\\\\Comptine.exe$\\\" $\\\"%1$\\\"'
    System::Call 'Shell32::SHChangeNotify(i 0x08000000, i 0, p 0, p 0)'
  ")
  set(CPACK_NSIS_EXTRA_UNINSTALL_COMMANDS "
    DeleteRegKey HKCR '.comptine'
    DeleteRegKey HKCR 'Comptine.Budget'
    System::Call 'Shell32::SHChangeNotify(i 0x08000000, i 0, p 0, p 0)'
  ")
endif()

include(CPack)
