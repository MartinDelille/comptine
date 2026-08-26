qt_add_library(model STATIC)

qt_add_qml_module(model
    URI Comptine.Model
    VERSION 1.0
    OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/Comptine/Model
    NO_PLUGIN
    SOURCES
        Account.cpp Account.h
        Category.cpp Category.h
        Operation.cpp Operation.h
        Rule.cpp Rule.h
        PropertyMacros.h
)

target_link_libraries(model PUBLIC Qt6::Core Qt6::Qml)

target_include_directories(model PUBLIC ${PROJECT_SOURCE_DIR})
