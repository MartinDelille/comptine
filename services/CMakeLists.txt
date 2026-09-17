qt_add_library(services STATIC)

if(APPLE)
    set(FILE_COORDINATOR_SOURCE FileCoordinator.mm)
else()
    set(FILE_COORDINATOR_SOURCE FileCoordinator_fallback.cpp)
endif()

qt_add_qml_module(services
    URI services
    IMPORTS
        TARGET model
    SOURCES
        AppSettings.cpp AppSettings.h
        AppState.h
        BudgetData.cpp BudgetData.h
        CategoryController.cpp CategoryController.h
        ClipboardController.cpp ClipboardController.h
        CsvParser.h
        EvolutionController.cpp EvolutionController.h
        FileController.cpp FileController.h
        FileCoordinator.h ${FILE_COORDINATOR_SOURCE}
        RuleController.cpp RuleController.h
        RuleListModel.cpp RuleListModel.h
        ServiceForeigners.h
        TranslationManager.cpp TranslationManager.h
        UndoCommands.cpp UndoCommands.h
        UpdateController.cpp UpdateController.h
)

target_link_libraries(
    services
    PUBLIC
        Qt6::Core
        Qt6::Gui
        Qt6::Qml
        Qt6::Network
        yaml-cpp::yaml-cpp
        model
        comptine_utils
    PRIVATE project_warnings
)

if(APPLE)
    target_link_libraries(services PRIVATE "-framework Foundation")
endif()

target_include_directories(
    services
    PUBLIC
        ${PROJECT_SOURCE_DIR}
        ${PROJECT_BINARY_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_BINARY_DIR}
)
