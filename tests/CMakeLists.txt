# Unit tests
enable_testing()
find_package(Qt6 REQUIRED COMPONENTS Test)

# CsvParserTest - header-only library test
qt_add_executable(CsvParserTest CsvParserTest.cpp)
target_link_libraries(CsvParserTest PRIVATE Qt6::Test services editor)
add_test(NAME CsvParserTest COMMAND CsvParserTest)

# CategoryTest - unit tests for Category class
qt_add_executable(CategoryTest CategoryTest.cpp)
target_link_libraries(CategoryTest PRIVATE Qt6::Test services editor)
add_test(NAME CategoryTest COMMAND CategoryTest)

# RuleTest - unit tests for categorization rules
qt_add_executable(RuleTest RuleTest.cpp)
target_link_libraries(RuleTest PRIVATE Qt6::Test services editor)
add_test(NAME RuleTest COMMAND RuleTest)

# AccountEditorTest - account rename editing and undo behavior
qt_add_executable(AccountEditorTest AccountEditorTest.cpp)
target_link_libraries(AccountEditorTest PRIVATE Qt6::Test services editor)
add_test(NAME AccountEditorTest COMMAND AccountEditorTest)

# OperationEditorTest - operation editing, allocations, and undo behavior
qt_add_executable(OperationEditorTest OperationEditorTest.cpp)
target_link_libraries(OperationEditorTest PRIVATE Qt6::Test services editor)
add_test(NAME OperationEditorTest COMMAND OperationEditorTest)

# CategoryEditorTest - category editing, monthly amounts, and removal behavior
qt_add_executable(CategoryEditorTest CategoryEditorTest.cpp)
target_link_libraries(CategoryEditorTest PRIVATE Qt6::Test services editor)
add_test(NAME CategoryEditorTest COMMAND CategoryEditorTest)

# RuleControllerTest - rule management, matching, and operation navigation
qt_add_executable(RuleControllerTest RuleControllerTest.cpp)
target_link_libraries(RuleControllerTest PRIVATE Qt6::Test services editor)
add_test(NAME RuleControllerTest COMMAND RuleControllerTest)

# UndoCommandsTest - undo/redo command behavior across backend entities
qt_add_executable(UndoCommandsTest UndoCommandsTest.cpp)
target_link_libraries(UndoCommandsTest PRIVATE Qt6::Test services editor)
add_test(NAME UndoCommandsTest COMMAND UndoCommandsTest)

# AppSettingsTest - persisted preferences and recent file behavior
qt_add_executable(AppSettingsTest AppSettingsTest.cpp)
target_link_libraries(AppSettingsTest PRIVATE Qt6::Test services editor)
add_test(NAME AppSettingsTest COMMAND AppSettingsTest)

# UpdateControllerTest - update state, timing, and response handling
qt_add_executable(UpdateControllerTest UpdateControllerTest.cpp)
target_link_libraries(UpdateControllerTest PRIVATE Qt6::Test services editor)
add_test(NAME UpdateControllerTest COMMAND UpdateControllerTest)

# TranslationManagerTest - language branch and retranslation behavior
qt_add_executable(TranslationManagerTest TranslationManagerTest.cpp)
target_link_libraries(TranslationManagerTest PRIVATE Qt6::Test services editor)
add_test(NAME TranslationManagerTest COMMAND TranslationManagerTest)

# FileCoordinatorTest - coordinated file read success and error paths
qt_add_executable(FileCoordinatorTest FileCoordinatorTest.cpp)
target_link_libraries(FileCoordinatorTest PRIVATE Qt6::Test services editor)
add_test(NAME FileCoordinatorTest COMMAND FileCoordinatorTest)

# AccountTest - account model, balances, selection, and navigation
qt_add_executable(AccountTest AccountTest.cpp)
target_link_libraries(AccountTest PRIVATE Qt6::Test services editor)
add_test(NAME AccountTest COMMAND AccountTest)

# OperationTest - operation allocations, budget dates, and model behavior
qt_add_executable(OperationTest OperationTest.cpp)
target_link_libraries(OperationTest PRIVATE Qt6::Test services editor)
add_test(NAME OperationTest COMMAND OperationTest)

# FileControllerTest - integration test with all dependencies
qt_add_executable(FileControllerTest FileControllerTest.cpp)

qt_add_resources(
  FileControllerTest
  FileControllerTestResources
  PREFIX "/tests"
  FILES
  example.comptine
  old.comptine
  import1.csv
  import2.csv
  money.csv
  money2.csv
)

target_link_libraries(FileControllerTest PRIVATE Qt6::Test services editor)

add_test(NAME FileControllerTest COMMAND FileControllerTest)
