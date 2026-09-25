set(COMPTINE_CLAZY_CHECKS
    "qstring-allocations"
    CACHE STRING
    "Comma-separated Clazy checks to enable"
)
set(COMPTINE_CLAZY_EXPORT_FIXES
    ""
    CACHE FILEPATH
    "Optional path for Clazy fix-it output"
)
set(COMPTINE_CLAZY_STANDALONE_EXECUTABLE
    ""
    CACHE FILEPATH
    "clazy-standalone executable used for static analysis"
)
set(COMPTINE_CLAZY_CLANGXX_EXECUTABLE
    ""
    CACHE FILEPATH
    "clang++ executable used by clazy-standalone"
)

if(COMPTINE_CLAZY_STANDALONE_EXECUTABLE)
    set(CLAZY_STANDALONE_EXECUTABLE "${COMPTINE_CLAZY_STANDALONE_EXECUTABLE}")
else()
    find_program(CLAZY_STANDALONE_EXECUTABLE NAMES clazy-standalone REQUIRED)
    set(COMPTINE_CLAZY_STANDALONE_EXECUTABLE
        "${CLAZY_STANDALONE_EXECUTABLE}"
        CACHE FILEPATH
        "clazy-standalone executable used for static analysis"
        FORCE
    )
endif()

if(COMPTINE_CLAZY_CLANGXX_EXECUTABLE)
    set(CLAZY_CLANGXX_EXECUTABLE "${COMPTINE_CLAZY_CLANGXX_EXECUTABLE}")
else()
    find_program(
        CLAZY_CLANGXX_EXECUTABLE
        NAMES clang++
        HINTS
        ENV LLVM_ROOT
        /opt/homebrew/opt/llvm
        /usr/local/opt/llvm
        PATH_SUFFIXES bin
        REQUIRED
    )
    set(COMPTINE_CLAZY_CLANGXX_EXECUTABLE
        "${CLAZY_CLANGXX_EXECUTABLE}"
        CACHE FILEPATH
        "clang++ executable used by clazy-standalone"
        FORCE
    )
endif()

execute_process(
    COMMAND "${COMPTINE_CLAZY_CLANGXX_EXECUTABLE}" -print-resource-dir
    OUTPUT_VARIABLE CLAZY_RESOURCE_DIR
    OUTPUT_STRIP_TRAILING_WHITESPACE
    RESULT_VARIABLE CLAZY_RESOURCE_DIR_RESULT
    ERROR_QUIET
)
if(CLAZY_RESOURCE_DIR_RESULT)
    message(FATAL_ERROR "Could not determine the Clang resource directory")
endif()

execute_process(
    COMMAND "${COMPTINE_CLAZY_STANDALONE_EXECUTABLE}" --version
    OUTPUT_VARIABLE CLAZY_VERSION
    OUTPUT_STRIP_TRAILING_WHITESPACE
    ERROR_QUIET
)
execute_process(
    COMMAND "${COMPTINE_CLAZY_CLANGXX_EXECUTABLE}" --version
    OUTPUT_VARIABLE CLAZY_CLANGXX_VERSION
    OUTPUT_STRIP_TRAILING_WHITESPACE
    ERROR_QUIET
)
message(
    STATUS
    "Using clazy-standalone: ${COMPTINE_CLAZY_STANDALONE_EXECUTABLE}"
)
message(STATUS "Clazy version: ${CLAZY_VERSION}")
message(STATUS "Clazy clang++: ${COMPTINE_CLAZY_CLANGXX_EXECUTABLE}")
message(STATUS "Clazy clang++ version: ${CLAZY_CLANGXX_VERSION}")

file(
    GLOB_RECURSE COMPTINE_CPP_SOURCES
    CONFIGURE_DEPENDS
    "${PROJECT_SOURCE_DIR}/*.cpp"
)
list(FILTER COMPTINE_CPP_SOURCES EXCLUDE REGEX "/build/")

set(CLAZY_EXPORT_FIXES_ARGUMENTS)
if(COMPTINE_CLAZY_EXPORT_FIXES)
    list(
        APPEND CLAZY_EXPORT_FIXES_ARGUMENTS
        "-export-fixes=${COMPTINE_CLAZY_EXPORT_FIXES}"
    )
endif()

add_custom_target(
    clazy-analysis
    COMMAND
        "${CMAKE_COMMAND}" -E env "CLANGXX=${COMPTINE_CLAZY_CLANGXX_EXECUTABLE}"
        "${COMPTINE_CLAZY_STANDALONE_EXECUTABLE}"
        "-checks=${COMPTINE_CLAZY_CHECKS}" "-p=${CMAKE_BINARY_DIR}"
        "-extra-arg-before=-resource-dir=${CLAZY_RESOURCE_DIR}"
        ${CLAZY_EXPORT_FIXES_ARGUMENTS} ${COMPTINE_CPP_SOURCES}
    WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
    USES_TERMINAL
    COMMENT "Running Clazy analysis"
)

add_dependencies(
    clazy-analysis
    Comptine
    model
    services
    editor
    tst_qmltests
    CsvParserTest
    CategoryTest
    EvolutionControllerTest
    RuleTest
    AccountEditorTest
    OperationEditorTest
    CategoryEditorTest
    RuleControllerTest
    UndoCommandsTest
    AppSettingsTest
    UpdateControllerTest
    TranslationManagerTest
    FileCoordinatorTest
    AccountTest
    OperationTest
    OperationFilterModelTest
    CategoryControllerTest
    BudgetDataTest
    ClipboardControllerTest
    RuleEditorTest
    FileControllerTest
)
