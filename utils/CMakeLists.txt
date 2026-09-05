qt_add_library(comptine_utils INTERFACE)

target_sources(comptine_utils INTERFACE ForeignBase.h PropertyMacros.h)

target_include_directories(comptine_utils INTERFACE ${PROJECT_SOURCE_DIR})
