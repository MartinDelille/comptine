qt_add_library(editor STATIC)

qt_add_qml_module(editor
    URI editor
    SOURCES
        AccountEditor.cpp AccountEditor.h
        CategoryEditor.cpp CategoryEditor.h
        ImportEditor.cpp ImportEditor.h
        OperationEditor.cpp OperationEditor.h
        RuleEditor.cpp RuleEditor.h
        EditorForeigners.h
)

target_link_libraries(
    editor
    PUBLIC Qt6::Core Qt6::Qml services model comptine_utils
    PRIVATE project_warnings
)

target_include_directories(editor PUBLIC ${PROJECT_SOURCE_DIR})
