qt_add_library(model STATIC)

qt_add_qml_module(model
    URI model
    VERSION 1.0
    SOURCES
        Account.cpp Account.h
        Category.cpp Category.h
        Operation.cpp Operation.h
        Rule.cpp Rule.h
)

target_link_libraries(
    model
    PUBLIC Qt6::Core Qt6::Qml comptine_utils
    PRIVATE project_warnings
)

target_include_directories(model PUBLIC ${PROJECT_SOURCE_DIR})
