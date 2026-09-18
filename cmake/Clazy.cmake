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

find_program(CLAZY_STANDALONE_EXECUTABLE NAMES clazy-standalone REQUIRED)

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

execute_process(
    COMMAND "${CLAZY_CLANGXX_EXECUTABLE}" -print-resource-dir
    OUTPUT_VARIABLE CLAZY_RESOURCE_DIR
    OUTPUT_STRIP_TRAILING_WHITESPACE
    RESULT_VARIABLE CLAZY_RESOURCE_DIR_RESULT
    ERROR_QUIET
)
if(CLAZY_RESOURCE_DIR_RESULT)
    message(FATAL_ERROR "Could not determine the Clang resource directory")
endif()

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
        "${CMAKE_COMMAND}" -E env "CLANGXX=${CLAZY_CLANGXX_EXECUTABLE}"
        "${CLAZY_STANDALONE_EXECUTABLE}" "-checks=${COMPTINE_CLAZY_CHECKS}"
        "-p=${CMAKE_BINARY_DIR}"
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
