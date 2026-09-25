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
find_package(OpenSSL REQUIRED)

find_package(Qt6 6.8 REQUIRED COMPONENTS Quick LinguistTools Network QuickTest)

qt_standard_project_setup(REQUIRES 6.8)

option(COMPTINE_ENABLE_COVERAGE "Enable C++ code coverage instrumentation" OFF)
option(
    COMPTINE_ENABLE_UPDATE_TEST_OVERRIDE
    "Enable environment overrides for update testing"
    OFF
)
option(COMPTINE_ENABLE_CLANG_TIDY "Enable clang-tidy static analysis" OFF)
option(COMPTINE_ENABLE_CLAZY "Enable Clazy static analysis" OFF)
set(COMPTINE_CLANG_TIDY_EXECUTABLE
    ""
    CACHE FILEPATH
    "clang-tidy executable used for static analysis"
)

if(COMPTINE_ENABLE_CLANG_TIDY)
    if(COMPTINE_CLANG_TIDY_EXECUTABLE)
        set(CLANG_TIDY_EXECUTABLE "${COMPTINE_CLANG_TIDY_EXECUTABLE}")
    else()
        find_program(CLANG_TIDY_EXECUTABLE NAMES clang-tidy)
    endif()
    if(NOT CLANG_TIDY_EXECUTABLE)
        message(
            FATAL_ERROR
            "COMPTINE_ENABLE_CLANG_TIDY is ON, but clang-tidy was not found"
        )
    endif()

    set(CMAKE_CXX_CLANG_TIDY
        "${CLANG_TIDY_EXECUTABLE}"
        CACHE STRING
        "clang-tidy command used for C++ compilation"
        FORCE
    )
    execute_process(
        COMMAND "${CLANG_TIDY_EXECUTABLE}" --version
        OUTPUT_VARIABLE CLANG_TIDY_VERSION
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET
    )
    message(STATUS "Using clang-tidy: ${CLANG_TIDY_EXECUTABLE}")
    message(STATUS "clang-tidy version: ${CLANG_TIDY_VERSION}")
endif()

if(COMPTINE_ENABLE_COVERAGE)
    if(MSVC)
        message(FATAL_ERROR "COMPTINE_ENABLE_COVERAGE requires GCC or Clang")
    endif()

    add_compile_options(--coverage -O0 -g)
    add_link_options(--coverage)
endif()

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
add_subdirectory(ui/evolution)

add_subdirectory(app)

include(CTest)
add_subdirectory(tests)

if(COMPTINE_ENABLE_CLAZY)
    include(cmake/Clazy.cmake)
endif()

include(cmake/packaging.cmake)
