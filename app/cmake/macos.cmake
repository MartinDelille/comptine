# macOS application bundle and icon configuration.

set(APP_ICON_SVG ${PROJECT_SOURCE_DIR}/comptine.svg)
set(APP_ICON_SMALL_SVG ${PROJECT_SOURCE_DIR}/docs/assets/comptine-small.svg)
set(GENERATE_ICON ${PROJECT_SOURCE_DIR}/scripts/generate_icon.py)
set(APP_ICON_ICNS ${CMAKE_CURRENT_BINARY_DIR}/comptine.icns)

add_custom_command(
    OUTPUT ${APP_ICON_ICNS}
    COMMAND
        uv run python ${GENERATE_ICON} --os macos --svg ${APP_ICON_SVG}
        --svg-small ${APP_ICON_SMALL_SVG} --out ${APP_ICON_ICNS}
    DEPENDS ${APP_ICON_SVG} ${APP_ICON_SMALL_SVG} ${GENERATE_ICON}
    COMMENT "Generating macOS icon from comptine.svg and comptine-small.svg"
)
add_custom_target(MacOSIcon DEPENDS ${APP_ICON_ICNS})
add_dependencies(Comptine MacOSIcon)

set_target_properties(
    Comptine
    PROPERTIES
        MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
        MACOSX_BUNDLE_GUI_IDENTIFIER org.delille.martin.Comptine
        MACOSX_BUNDLE_SHORT_VERSION_STRING
            ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
        MACOSX_BUNDLE_ICON_FILE comptine.icns
        MACOSX_BUNDLE_INFO_PLIST ${PROJECT_SOURCE_DIR}/Info.plist.in
        MACOSX_BUNDLE TRUE
)

add_custom_command(
    TARGET Comptine
    POST_BUILD
    COMMAND
        ${CMAKE_COMMAND} -E make_directory
        "$<TARGET_FILE_DIR:Comptine>/../Resources"
    COMMAND
        ${CMAKE_COMMAND} -E copy_if_different ${APP_ICON_ICNS}
        "$<TARGET_FILE_DIR:Comptine>/../Resources/comptine.icns"
    COMMENT "Installing macOS icon into application bundle"
)
