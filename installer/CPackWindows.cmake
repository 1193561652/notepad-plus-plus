# Windows NSIS installer for the Notepad++ Qt port.
# It intentionally keeps the original install flow (directory, components,
# shortcuts and uninstaller) while omitting bundled plugins, shell extensions
# and online-updater behavior that the Qt port does not ship.

include("${CMAKE_SOURCE_DIR}/installer_common/CPackCommon.cmake")
include("${CMAKE_SOURCE_DIR}/installer_common/LocalizationComponents.cmake")

install(TARGETS notepadpp-qt npp-plugin-updater
    RUNTIME DESTINATION "."
    COMPONENT Application)

# Match the program-relative resource layout used by original Notepad++.
npp_install_localizations("localization" COMPONENTIZED)
install(DIRECTORY "${CMAKE_SOURCE_DIR}/resources/functionList/"
    DESTINATION "functionList"
    COMPONENT Application
    FILES_MATCHING PATTERN "*.xml")
install(FILES
    "${CMAKE_SOURCE_DIR}/LICENSE"
    "${CMAKE_SOURCE_DIR}/QT_PORT_NOTICE.md"
    "${CMAKE_SOURCE_DIR}/README.md"
    DESTINATION "."
    COMPONENT Application)
install(FILES "${CMAKE_SOURCE_DIR}/resources/icons/npp.ico"
    DESTINATION "."
    COMPONENT Application)

# The build already identifies the selected Qt kit. Install only that kit's
# runtime DLLs and plugins so the result remains self-contained.
foreach(_npp_qt_runtime Core Gui Widgets Network PrintSupport Xml)
    install(FILES "$<TARGET_FILE:Qt5::${_npp_qt_runtime}>"
        DESTINATION "."
        COMPONENT Application)
endforeach()
if(MINGW AND NPP_MINGW_RUNTIME_DLLS)
    install(FILES ${NPP_MINGW_RUNTIME_DLLS}
        DESTINATION "."
        COMPONENT Application)
endif()
foreach(_npp_qt_plugin_directory platforms imageformats)
    if(EXISTS "${NPP_QT_PLUGIN_DIR}/${_npp_qt_plugin_directory}")
        install(DIRECTORY
            "${NPP_QT_PLUGIN_DIR}/${_npp_qt_plugin_directory}/"
            DESTINATION "${_npp_qt_plugin_directory}"
            COMPONENT Application
            FILES_MATCHING PATTERN "*.dll")
    endif()
endforeach()

set(CPACK_GENERATOR "NSIS")
set(CPACK_PACKAGE_FILE_NAME
    "npp.${PROJECT_VERSION}-qt1.Installer.${CMAKE_SYSTEM_PROCESSOR}")
set(CPACK_PACKAGE_INSTALL_DIRECTORY "Notepad++")
set(CPACK_PACKAGE_INSTALL_REGISTRY_KEY "Notepad++ Qt ${PROJECT_VERSION}")
set(CPACK_PACKAGE_EXECUTABLES "notepad++;Notepad++")
set(CPACK_CREATE_DESKTOP_LINKS "notepad++")

set(CPACK_COMPONENT_APPLICATION_DISPLAY_NAME "Notepad++")
set(CPACK_COMPONENT_APPLICATION_DESCRIPTION
    "Notepad++ Qt application, required Qt runtime and core resources.")
set(CPACK_COMPONENT_APPLICATION_REQUIRED TRUE)
set(CPACK_COMPONENT_GROUP_LOCALIZATION_DISPLAY_NAME "Localization")
set(CPACK_COMPONENT_GROUP_LOCALIZATION_DESCRIPTION
    "Optional official Notepad++ user-interface language files.")
set(CPACK_COMPONENTS_ALL Application ${NPP_OPTIONAL_LANGUAGE_COMPONENTS})

set(CPACK_NSIS_PACKAGE_NAME "Notepad++ ${PROJECT_VERSION} Qt")
set(CPACK_NSIS_DISPLAY_NAME "Notepad++")
set(CPACK_NSIS_INSTALLED_ICON_NAME "npp.ico")
set(CPACK_NSIS_EXECUTABLES_DIRECTORY ".")
set(CPACK_NSIS_MUI_ICON "${CMAKE_SOURCE_DIR}/resources/icons/npp.ico")
set(CPACK_NSIS_MUI_UNIICON "${CMAKE_SOURCE_DIR}/resources/icons/npp.ico")
set(CPACK_NSIS_MUI_FINISHPAGE_RUN "notepad++.exe")
set(CPACK_NSIS_ENABLE_UNINSTALL_BEFORE_INSTALL ON)
set(CPACK_NSIS_MANIFEST_DPI_AWARE ON)
set(CPACK_NSIS_COMPRESSOR "/SOLID lzma")
set(CPACK_NSIS_HELP_LINK
    "https://github.com/1193561652/notepad-plus-plus/tree/qt-port")
set(CPACK_NSIS_URL_INFO_ABOUT
    "https://github.com/1193561652/notepad-plus-plus/tree/qt-port")
set(CPACK_NSIS_CONTACT "1193561652@users.noreply.github.com")
set(CPACK_NSIS_UNINSTALL_NAME "Uninstall Notepad++")

include(CPack)
