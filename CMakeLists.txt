cmake_minimum_required(VERSION 3.16)

project(
  Comptine
  VERSION 0.1
  LANGUAGES CXX)

set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS Quick)

qt_standard_project_setup(REQUIRES 6.8)

qt_add_executable(Comptine 
  main.cpp
  Operation.h
  Operation.cpp
  Account.h
  Account.cpp
  Category.h
  Category.cpp
  BudgetData.h
  BudgetData.cpp
)

# Generate macOS icon from PNG at build time
set(APP_ICON_PNG ${CMAKE_CURRENT_SOURCE_DIR}/comptine.png)
set(APP_ICON_ICNS ${CMAKE_CURRENT_BINARY_DIR}/comptine.icns)
set(APP_ICONSET_DIR ${CMAKE_CURRENT_BINARY_DIR}/comptine.iconset)

add_custom_command(
  OUTPUT ${APP_ICON_ICNS}
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
  DEPENDS ${APP_ICON_PNG}
  COMMENT "Generating macOS icon from comptine.png"
)

set_source_files_properties(${APP_ICON_ICNS} PROPERTIES MACOSX_PACKAGE_LOCATION "Resources")
target_sources(Comptine PRIVATE ${APP_ICON_ICNS})

qt_add_qml_module(Comptine URI Comptine QML_FILES
    Main.qml
    BalanceHeader.qml
    OperationDelegate.qml
    OperationList.qml
    OperationDetails.qml
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
             MACOSX_BUNDLE TRUE
             WIN32_EXECUTABLE TRUE)

target_link_libraries(Comptine PRIVATE Qt6::Quick)

include(GNUInstallDirs)
install(
  TARGETS Comptine
  BUNDLE DESTINATION .
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
