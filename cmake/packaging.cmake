set(CPACK_PACKAGE_NAME "Comptine")
set(CPACK_PACKAGE_VENDOR "Martin Delille")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Personal budget management application")
set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
set(CPACK_PACKAGE_VERSION_MAJOR ${PROJECT_VERSION_MAJOR})
set(CPACK_PACKAGE_VERSION_MINOR ${PROJECT_VERSION_MINOR})
set(CPACK_PACKAGE_INSTALL_DIRECTORY "Comptine")

if(APPLE)
    # Keep the license as an installed file; do not make CPack build a DMG
    # software-license agreement from it.
    set(CPACK_DMG_SLA_USE_RESOURCE_FILE_LICENSE OFF)
elseif(WIN32 AND EXISTS "${PROJECT_SOURCE_DIR}/LICENSE.md")
    set(CPACK_RESOURCE_FILE_LICENSE "${PROJECT_SOURCE_DIR}/LICENSE.md")
endif()

install(FILES "${PROJECT_SOURCE_DIR}/LICENSE.md" DESTINATION .)

if(APPLE)
    include(${PROJECT_SOURCE_DIR}/cmake/packaging/macos.cmake)
elseif(WIN32)
    include(${PROJECT_SOURCE_DIR}/cmake/packaging/windows.cmake)
endif()

include(CPack)
