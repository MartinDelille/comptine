cmake_minimum_required(VERSION 3.16)

project(
  Comptine
  VERSION 0.1
  LANGUAGES CXX)

set(CMAKE_CXX_STANDARD_REQUIRED ON)

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

find_package(Qt6 REQUIRED COMPONENTS Quick LinguistTools)

qt_standard_project_setup(REQUIRES 6.8)

qt_add_executable(Comptine
  main.cpp
  PropertyMacros.h
  Operation.h
  Operation.cpp
  Account.h
  Account.cpp
  Category.h
  Category.cpp
  BudgetData.h
  BudgetData.cpp
  OperationListModel.h
  OperationListModel.cpp
  AccountListModel.h
  AccountListModel.cpp
  AppSettings.h
  AppSettings.cpp
  UndoCommands.h
  UndoCommands.cpp
)

# Generate platform-specific icons from SVG at build time
set(APP_ICON_SVG ${CMAKE_CURRENT_SOURCE_DIR}/comptine.svg)

if(APPLE)
  set(APP_ICON_PNG ${CMAKE_CURRENT_BINARY_DIR}/comptine.png)
  set(APP_ICON_ICNS ${CMAKE_CURRENT_BINARY_DIR}/comptine.icns)
  set(APP_ICONSET_DIR ${CMAKE_CURRENT_BINARY_DIR}/comptine.iconset)

  # Use ImageMagick to render SVG to PNG with transparency preserved
  add_custom_command(
    OUTPUT ${APP_ICON_ICNS}
    COMMAND magick -background none ${APP_ICON_SVG} -resize 1024x1024 ${APP_ICON_PNG}
    COMMAND ${CMAKE_COMMAND} -E make_directory ${APP_ICONSET_DIR}
    COMMAND sips -z 16 16 ${APP_ICON_PNG} --out ${APP_ICONSET_DIR}/icon_16x16.png
    COMMAND sips -z 32 32 ${APP_ICON_PNG} --out ${APP_ICONSET_DIR}/icon_16x16@2x.png
    COMMAND sips -z 32 32 ${APP_ICON_PNG} --out ${APP_ICONSET_DIR}/icon_32x32.png
    COMMAND sips -z 64 64 ${APP_ICON_PNG} --out ${APP_ICONSET_DIR}/icon_32x32@2x.png
    COMMAND sips -z 128 128 ${APP_ICON_PNG} --out ${APP_ICONSET_DIR}/icon_128x128.png
    COMMAND sips -z 256 256 ${APP_ICON_PNG} --out ${APP_ICONSET_DIR}/icon_128x128@2x.png
    COMMAND sips -z 256 256 ${APP_ICON_PNG} --out ${APP_ICONSET_DIR}/icon_256x256.png
    COMMAND sips -z 512 512 ${APP_ICON_PNG} --out ${APP_ICONSET_DIR}/icon_256x256@2x.png
    COMMAND sips -z 512 512 ${APP_ICON_PNG} --out ${APP_ICONSET_DIR}/icon_512x512.png
    COMMAND sips -z 1024 1024 ${APP_ICON_PNG} --out ${APP_ICONSET_DIR}/icon_512x512@2x.png
    COMMAND iconutil -c icns ${APP_ICONSET_DIR} -o ${APP_ICON_ICNS}
    DEPENDS ${APP_ICON_SVG}
    COMMENT "Generating macOS icon from comptine.svg"
  )

  set_source_files_properties(${APP_ICON_ICNS} PROPERTIES MACOSX_PACKAGE_LOCATION "Resources")
  target_sources(Comptine PRIVATE ${APP_ICON_ICNS})
elseif(WIN32)
  set(APP_ICON_ICO ${CMAKE_CURRENT_BINARY_DIR}/comptine.ico)
  set(APP_ICON_RC ${CMAKE_CURRENT_BINARY_DIR}/comptine.rc)

  # Use ImageMagick to create multi-resolution ICO file
  add_custom_command(
    OUTPUT ${APP_ICON_ICO}
    COMMAND magick -background none ${APP_ICON_SVG} -define icon:auto-resize=256,128,64,48,32,16 ${APP_ICON_ICO}
    DEPENDS ${APP_ICON_SVG}
    COMMENT "Generating Windows icon from comptine.svg"
  )

  # Generate RC file that references the icon
  file(WRITE ${APP_ICON_RC} "IDI_ICON1 ICON \"comptine.ico\"\n")

  # Add custom target to ensure icon is built before the RC file is compiled
  add_custom_target(WindowsIcon DEPENDS ${APP_ICON_ICO})
  add_dependencies(Comptine WindowsIcon)

  target_sources(Comptine PRIVATE ${APP_ICON_RC})
endif()

set_source_files_properties(Theme.qml PROPERTIES QT_QML_SINGLETON_TYPE TRUE)

qt_add_qml_module(Comptine URI Comptine QML_FILES
    Main.qml
    AmountField.qml
    BalanceHeader.qml
    BudgetView.qml
    CategoryDetailView.qml
    EditBudgetDateDialog.qml
    EditCategoryDialog.qml
    ImportDialog.qml
    MonthSelector.qml
    OperationDelegate.qml
    OperationDetails.qml
    OperationList.qml
    OperationView.qml
    PreferencesDialog.qml
    SplitOperationDialog.qml
    Theme.qml
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

target_link_libraries(Comptine PRIVATE Qt6::Quick ryml::ryml)

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

  # Include the deployed app bundle
  set(CPACK_BUNDLE_NAME "Comptine")

elseif(WIN32)
  # Windows: NSIS installer
  set(CPACK_GENERATOR "NSIS")

  # Basic NSIS settings
  set(CPACK_NSIS_DISPLAY_NAME "Comptine")
  set(CPACK_NSIS_PACKAGE_NAME "Comptine")
  set(CPACK_NSIS_INSTALLED_ICON_NAME "bin\\\\Comptine.exe")
  set(CPACK_NSIS_MUI_ICON "${CMAKE_CURRENT_BINARY_DIR}/comptine.ico")
  set(CPACK_NSIS_MUI_UNIICON "${CMAKE_CURRENT_BINARY_DIR}/comptine.ico")

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
