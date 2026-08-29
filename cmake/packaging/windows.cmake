set(CPACK_GENERATOR "NSIS")
set(CPACK_PACKAGE_FILE_NAME "${CPACK_PACKAGE_NAME}-Windows")
set(CPACK_NSIS_DISPLAY_NAME "Comptine")
set(CPACK_NSIS_PACKAGE_NAME "Comptine")
set(CPACK_NSIS_INSTALLED_ICON_NAME "bin\\\\Comptine.exe")
set(CPACK_NSIS_MUI_ICON "${PROJECT_BINARY_DIR}/app/comptine.ico")
set(CPACK_NSIS_MUI_UNIICON "${PROJECT_BINARY_DIR}/app/comptine.ico")
set(CPACK_NSIS_MUI_FINISHPAGE_RUN "..\\\\bin\\\\Comptine.exe")

set(CPACK_NSIS_CREATE_ICONS_EXTRA
    "CreateShortCut '$SMPROGRAMS\\\\$STARTMENU_FOLDER\\\\Comptine.lnk' '$INSTDIR\\\\bin\\\\Comptine.exe'
 CreateShortCut '$DESKTOP\\\\Comptine.lnk' '$INSTDIR\\\\bin\\\\Comptine.exe'"
)
set(CPACK_NSIS_DELETE_ICONS_EXTRA
    "Delete '$SMPROGRAMS\\\\$START_MENU\\\\Comptine.lnk'
 Delete '$DESKTOP\\\\Comptine.lnk'"
)

set(CPACK_NSIS_HELP_LINK "https://github.com/mart1n/Comptine")
set(CPACK_NSIS_URL_INFO_ABOUT "https://github.com/mart1n/Comptine")
set(CPACK_NSIS_MODIFY_PATH OFF)
set(CPACK_NSIS_ENABLE_UNINSTALL_BEFORE_INSTALL ON)

set(CPACK_NSIS_EXTRA_INSTALL_COMMANDS
    "
WriteRegStr HKCR '.comptine' '' 'Comptine.Budget'
WriteRegStr HKCR 'Comptine.Budget' '' 'Comptine Budget File'
WriteRegStr HKCR 'Comptine.Budget\\\\DefaultIcon' '' '$INSTDIR\\\\bin\\\\Comptine.exe,0'
WriteRegStr HKCR 'Comptine.Budget\\\\shell\\\\open\\\\command' '' '$\\\"$INSTDIR\\\\bin\\\\Comptine.exe$\\\" $\\\"%1$\\\"'
System::Call 'Shell32::SHChangeNotify(i 0x08000000, i 0, p 0, p 0)'
"
)
set(CPACK_NSIS_EXTRA_UNINSTALL_COMMANDS
    "
DeleteRegKey HKCR '.comptine'
DeleteRegKey HKCR 'Comptine.Budget'
System::Call 'Shell32::SHChangeNotify(i 0x08000000, i 0, p 0, p 0)'
"
)
