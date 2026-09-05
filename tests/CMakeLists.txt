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
