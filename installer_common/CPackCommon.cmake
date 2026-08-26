# Shared package metadata for the Windows, Ubuntu and future macOS installers.
# Platform-specific installer files should include this before include(CPack).

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
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_SOURCE_DIR}/LICENSE")
