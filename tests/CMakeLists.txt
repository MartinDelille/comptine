# Unit tests
enable_testing()
find_package(Qt6 REQUIRED COMPONENTS Test)

add_library(FileCoordinatorTestSupport INTERFACE)
target_link_libraries(
    FileCoordinatorTestSupport
    INTERFACE libFileCoordinatorFallback
)

# CsvParserTest - header-only library test
qt_add_executable(CsvParserTest CsvParserTest.cpp)
target_link_libraries(
    CsvParserTest
    PRIVATE Qt6::Test libComptine FileCoordinatorTestSupport
)
add_test(NAME CsvParserTest COMMAND CsvParserTest)

# CategoryTest - unit tests for Category class
qt_add_executable(CategoryTest CategoryTest.cpp)
target_link_libraries(
    CategoryTest
    PRIVATE Qt6::Test libComptine FileCoordinatorTestSupport
)
add_test(NAME CategoryTest COMMAND CategoryTest)

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

target_link_libraries(
    FileControllerTest
    PRIVATE Qt6::Test libComptine FileCoordinatorTestSupport
)

add_test(NAME FileControllerTest COMMAND FileControllerTest)
