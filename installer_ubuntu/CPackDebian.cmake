# Debian package layout for the Notepad++ Qt port.
# Use the Filesystem Hierarchy Standard while the runtime resolver preserves
# the original program-relative layout for Windows and portable deployments.

include("${CMAKE_SOURCE_DIR}/installer_common/CPackCommon.cmake")
include("${CMAKE_SOURCE_DIR}/installer_common/LocalizationComponents.cmake")

set(NPP_DEBIAN_DATA_ROOT
    "${CMAKE_INSTALL_DATADIR}/notepad-plus-plus-qt")
set(NPP_DEBIAN_LIBEXEC_ROOT
    "${CMAKE_INSTALL_LIBEXECDIR}/notepad-plus-plus-qt")

install(TARGETS notepadpp-qt
    RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
)
install(TARGETS npp-plugin-updater
    RUNTIME DESTINATION "${NPP_DEBIAN_LIBEXEC_ROOT}"
)
npp_install_localizations("${NPP_DEBIAN_DATA_ROOT}/localization")
install(DIRECTORY "${CMAKE_SOURCE_DIR}/resources/functionList/"
    DESTINATION "${NPP_DEBIAN_DATA_ROOT}/functionList"
    FILES_MATCHING PATTERN "*.xml"
)
install(FILES
    "${CMAKE_SOURCE_DIR}/LICENSE"
    "${CMAKE_SOURCE_DIR}/QT_PORT_NOTICE.md"
    "${CMAKE_SOURCE_DIR}/README.md"
    DESTINATION "${CMAKE_INSTALL_DATADIR}/doc/notepad-plus-plus-qt"
)
install(FILES "${CMAKE_SOURCE_DIR}/installer_ubuntu/notepad-plus-plus.png"
    DESTINATION "${CMAKE_INSTALL_DATADIR}/icons/hicolor/48x48/apps"
)
install(FILES
    "${CMAKE_SOURCE_DIR}/installer_ubuntu/notepad-plus-plus.desktop"
    DESTINATION "${CMAKE_INSTALL_DATADIR}/applications"
)

set(CPACK_GENERATOR "DEB")
set(CPACK_DEBIAN_PACKAGE_RELEASE "qt2")
set(CPACK_DEBIAN_PACKAGE_SECTION "editors")
set(CPACK_DEBIAN_PACKAGE_PRIORITY "optional")
set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON)
set(CPACK_DEBIAN_PACKAGE_CONTROL_STRICT_PERMISSION TRUE)
set(CPACK_DEBIAN_FILE_NAME DEB-DEFAULT)
set(CPACK_PACKAGING_INSTALL_PREFIX "/usr")

include(CPack)
