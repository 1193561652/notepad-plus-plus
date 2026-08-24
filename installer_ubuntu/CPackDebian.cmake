# Debian package layout for the Notepad++ Qt port.
# Keep the application and its program-relative resources together, matching
# the original Notepad++ runtime directory model.

set(NPP_DEBIAN_APPLICATION_ROOT "opt/notepad-plus-plus")

install(TARGETS notepadpp-qt npp-plugin-updater
    RUNTIME DESTINATION "${NPP_DEBIAN_APPLICATION_ROOT}"
)
install(DIRECTORY "${CMAKE_SOURCE_DIR}/resources/nativeLang/"
    DESTINATION "${NPP_DEBIAN_APPLICATION_ROOT}/localization"
    FILES_MATCHING PATTERN "*.xml"
)
install(DIRECTORY "${CMAKE_SOURCE_DIR}/resources/functionList/"
    DESTINATION "${NPP_DEBIAN_APPLICATION_ROOT}/functionList"
    FILES_MATCHING PATTERN "*.xml"
)
install(FILES
    "${CMAKE_SOURCE_DIR}/LICENSE"
    "${CMAKE_SOURCE_DIR}/QT_PORT_NOTICE.md"
    "${CMAKE_SOURCE_DIR}/README.md"
    DESTINATION "usr/share/doc/notepad-plus-plus-qt"
)
install(FILES "${CMAKE_SOURCE_DIR}/resources/icons/npp.ico"
    DESTINATION "${NPP_DEBIAN_APPLICATION_ROOT}"
)
install(PROGRAMS "${CMAKE_SOURCE_DIR}/installer_ubuntu/notepad++"
    DESTINATION "usr/bin"
)
install(FILES
    "${CMAKE_SOURCE_DIR}/installer_ubuntu/notepad-plus-plus.desktop"
    DESTINATION "usr/share/applications"
)

set(CPACK_GENERATOR "DEB")
set(CPACK_PACKAGE_NAME "notepad-plus-plus-qt")
set(CPACK_PACKAGE_VENDOR "Jiang Liwei")
set(CPACK_PACKAGE_CONTACT
    "Jiang Liwei <1193561652@users.noreply.github.com>")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY
    "Notepad++ Qt port for Windows, Ubuntu and macOS")
set(CPACK_PACKAGE_DESCRIPTION
    "An independent Qt port of Notepad++ v8.4.6. The port preserves the original application name and compatible configuration formats while replacing platform-specific implementation with Qt and standard C++.")
set(CPACK_PACKAGE_HOMEPAGE_URL
    "https://github.com/1193561652/notepad-plus-plus/tree/qt-port")
set(CPACK_PACKAGE_VERSION "${PROJECT_VERSION}")
set(CPACK_DEBIAN_PACKAGE_RELEASE "qt1")
set(CPACK_DEBIAN_PACKAGE_SECTION "editors")
set(CPACK_DEBIAN_PACKAGE_PRIORITY "optional")
set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON)
set(CPACK_DEBIAN_PACKAGE_CONTROL_STRICT_PERMISSION TRUE)
set(CPACK_DEBIAN_FILE_NAME DEB-DEFAULT)
set(CPACK_PACKAGING_INSTALL_PREFIX "/")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_SOURCE_DIR}/LICENSE")

include(CPack)
