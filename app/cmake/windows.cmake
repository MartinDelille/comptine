# Windows executable icon and Qt deployment configuration.

set(APP_ICON_SVG ${PROJECT_SOURCE_DIR}/comptine.svg)
set(APP_ICON_SMALL_SVG ${PROJECT_SOURCE_DIR}/docs/assets/comptine-small.svg)
set(GENERATE_ICON ${PROJECT_SOURCE_DIR}/scripts/generate_icon.py)
set(APP_ICON_ICO ${CMAKE_CURRENT_BINARY_DIR}/comptine.ico)
set(APP_ICON_RC ${CMAKE_CURRENT_BINARY_DIR}/comptine.rc)

add_custom_command(
    OUTPUT ${APP_ICON_ICO}
    COMMAND
        uv run python ${GENERATE_ICON} --os windows --svg ${APP_ICON_SVG}
        --svg-small ${APP_ICON_SMALL_SVG} --out ${APP_ICON_ICO}
    DEPENDS ${APP_ICON_SVG} ${APP_ICON_SMALL_SVG} ${GENERATE_ICON}
    COMMENT "Generating Windows icon from comptine.svg and comptine-small.svg"
)

file(WRITE ${APP_ICON_RC} "IDI_ICON1 ICON \"comptine.ico\"\n")
add_custom_target(WindowsIcon DEPENDS ${APP_ICON_ICO})
add_dependencies(Comptine WindowsIcon)
target_sources(Comptine PRIVATE ${APP_ICON_RC})

set_target_properties(Comptine PROPERTIES WIN32_EXECUTABLE TRUE)

qt_generate_deploy_qml_app_script(TARGET Comptine OUTPUT_SCRIPT deploy_script
                                  NO_UNSUPPORTED_PLATFORM_ERROR
)
install(SCRIPT ${deploy_script})
