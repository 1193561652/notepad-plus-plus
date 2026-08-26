# Debian package layout for the Notepad++ Qt port.
# Keep the application and its program-relative resources together, matching
# the original Notepad++ runtime directory model.

include("${CMAKE_SOURCE_DIR}/installer_common/CPackCommon.cmake")
include("${CMAKE_SOURCE_DIR}/installer_common/LocalizationComponents.cmake")

set(NPP_DEBIAN_APPLICATION_ROOT "opt/notepad-plus-plus")

install(TARGETS notepadpp-qt npp-plugin-updater
    RUNTIME DESTINATION "${NPP_DEBIAN_APPLICATION_ROOT}"
)
npp_install_localizations("${NPP_DEBIAN_APPLICATION_ROOT}/localization")
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
set(CPACK_DEBIAN_PACKAGE_RELEASE "qt1")
set(CPACK_DEBIAN_PACKAGE_SECTION "editors")
set(CPACK_DEBIAN_PACKAGE_PRIORITY "optional")
set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON)
set(CPACK_DEBIAN_PACKAGE_CONTROL_STRICT_PERMISSION TRUE)
set(CPACK_DEBIAN_FILE_NAME DEB-DEFAULT)
set(CPACK_PACKAGING_INSTALL_PREFIX "/")

include(CPack)
