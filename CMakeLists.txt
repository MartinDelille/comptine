cmake_minimum_required(VERSION 3.16)

project(
  Comptine
  VERSION 0.1
  LANGUAGES CXX)

set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS Quick)

qt_standard_project_setup(REQUIRES 6.8)

qt_add_executable(appComptine 
  main.cpp
  TransactionModel.h
  TransactionModel.cpp
)

qt_add_qml_module(appComptine URI Comptine QML_FILES Main.qml)

# Qt for iOS sets MACOSX_BUNDLE_GUI_IDENTIFIER automatically since Qt 6.1. If
# you are developing for iOS or macOS you should consider setting an explicit,
# fixed bundle identifier manually though.
set_target_properties(
  appComptine
  PROPERTIES # MACOSX_BUNDLE_GUI_IDENTIFIER com.example.appComptine
             MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
             MACOSX_BUNDLE_SHORT_VERSION_STRING
             ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
             MACOSX_BUNDLE TRUE
             WIN32_EXECUTABLE TRUE)

target_link_libraries(appComptine PRIVATE Qt6::Quick)

include(GNUInstallDirs)
install(
  TARGETS appComptine
  BUNDLE DESTINATION .
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
