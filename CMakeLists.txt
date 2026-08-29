cmake_minimum_required(VERSION 3.16)

include(cmake/ProjectVersion.cmake)
comptine_set_version()

project(Comptine VERSION ${VERSION_MAJOR}.${VERSION_MINOR} LANGUAGES CXX)

if(APPLE)
    include(cmake/platform/macos.cmake)
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Generate Version.h with version information
configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/Version.h.in
    ${CMAKE_CURRENT_BINARY_DIR}/Version.h
    @ONLY
)
# yaml-cpp (managed by Conan)
find_package(yaml-cpp REQUIRED)

find_package(Qt6 6.8 REQUIRED COMPONENTS Quick LinguistTools Network)

qt_standard_project_setup(REQUIRES 6.8)

add_library(project_warnings INTERFACE)

if(MSVC)
    target_compile_options(project_warnings INTERFACE /W4)
else()
    target_compile_options(
        project_warnings
        INTERFACE
            -Wall
            -Wextra
            -Werror
            -Wswitch-enum
            -Werror=implicit-fallthrough
    )
endif()

add_subdirectory(utils)
add_subdirectory(model)
add_subdirectory(services)
add_subdirectory(editor)

if(NOT MSVC)
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -g")
endif()

add_subdirectory(ui/common)
add_subdirectory(ui/rules)
add_subdirectory(ui/budget)
add_subdirectory(ui/operations)

add_subdirectory(app)

include(CTest)
add_subdirectory(tests)

include(cmake/packaging.cmake)
