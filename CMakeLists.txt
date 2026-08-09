cmake_minimum_required(VERSION 3.10)
project(NotepadPlusPlusQt VERSION 8.4.6 LANGUAGES CXX)

include(CTest)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOMOC_COMPILER_PREDEFINES OFF)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# 插件系统选项（默认关闭）
option(ENABLE_PLUGIN_SYSTEM "Enable plugin system support" OFF)

find_package(Qt5 REQUIRED COMPONENTS Core Gui Widgets Network PrintSupport Xml)

# Qt 路径（如果 CMake 找不到 Qt，取消注释下面这行）
# set(CMAKE_PREFIX_PATH "F:/Qt/Qt5.12.12/5.12.12/mingw73_32")

# Keep the editor stack aligned with the Notepad++ 8.4.6 baseline:
# Scintilla 5.3.0, Lexilla 5.1.9, and the original Boost.Regex adapter.
set(SCINTILLA_ROOT "${CMAKE_SOURCE_DIR}/third_party/scintilla")
set(SCINTILLA_QT_ROOT "${SCINTILLA_ROOT}/qt/ScintillaEditBase")
set(LEXILLA_ROOT "${CMAKE_SOURCE_DIR}/third_party/lexilla")
set(BOOSTREGEX_ROOT "${CMAKE_SOURCE_DIR}/third_party/boostregex")

file(GLOB NPP_LEXILLA_LEXER_SOURCES CONFIGURE_DEPENDS
    "${LEXILLA_ROOT}/lexers/*.cxx")
file(GLOB NPP_LEXILLA_LEXLIB_SOURCES CONFIGURE_DEPENDS
    "${LEXILLA_ROOT}/lexlib/*.cxx")

add_library(npp-lexilla STATIC
    ${NPP_LEXILLA_LEXER_SOURCES}
    ${NPP_LEXILLA_LEXLIB_SOURCES}
    "${LEXILLA_ROOT}/src/Lexilla.cxx"
)
target_compile_definitions(npp-lexilla PRIVATE SCI_LEXER)
target_include_directories(npp-lexilla
    PUBLIC
        "${LEXILLA_ROOT}/include"
        "${SCINTILLA_ROOT}/include"
    PRIVATE
        "${LEXILLA_ROOT}/lexlib"
        "${LEXILLA_ROOT}/src"
)

set(NPP_SCINTILLA_SOURCES
    ${SCINTILLA_QT_ROOT}/PlatQt.cpp
    ${SCINTILLA_QT_ROOT}/ScintillaQt.cpp
    ${SCINTILLA_QT_ROOT}/ScintillaEditBase.cpp
    ${SCINTILLA_ROOT}/src/AutoComplete.cxx
    ${SCINTILLA_ROOT}/src/CallTip.cxx
    ${SCINTILLA_ROOT}/src/CaseConvert.cxx
    ${SCINTILLA_ROOT}/src/CaseFolder.cxx
    ${SCINTILLA_ROOT}/src/CellBuffer.cxx
    ${SCINTILLA_ROOT}/src/ChangeHistory.cxx
    ${SCINTILLA_ROOT}/src/CharacterCategoryMap.cxx
    ${SCINTILLA_ROOT}/src/CharacterType.cxx
    ${SCINTILLA_ROOT}/src/CharClassify.cxx
    ${SCINTILLA_ROOT}/src/ContractionState.cxx
    ${SCINTILLA_ROOT}/src/DBCS.cxx
    ${SCINTILLA_ROOT}/src/Decoration.cxx
    ${SCINTILLA_ROOT}/src/Document.cxx
    ${SCINTILLA_ROOT}/src/EditModel.cxx
    ${SCINTILLA_ROOT}/src/Editor.cxx
    ${SCINTILLA_ROOT}/src/EditView.cxx
    ${SCINTILLA_ROOT}/src/Geometry.cxx
    ${SCINTILLA_ROOT}/src/Indicator.cxx
    ${SCINTILLA_ROOT}/src/KeyMap.cxx
    ${SCINTILLA_ROOT}/src/LineMarker.cxx
    ${SCINTILLA_ROOT}/src/MarginView.cxx
    ${SCINTILLA_ROOT}/src/PerLine.cxx
    ${SCINTILLA_ROOT}/src/PositionCache.cxx
    ${SCINTILLA_ROOT}/src/RESearch.cxx
    ${SCINTILLA_ROOT}/src/RunStyles.cxx
    ${SCINTILLA_ROOT}/src/ScintillaBase.cxx
    ${SCINTILLA_ROOT}/src/Selection.cxx
    ${SCINTILLA_ROOT}/src/Style.cxx
    ${SCINTILLA_ROOT}/src/UniConversion.cxx
    ${SCINTILLA_ROOT}/src/UniqueString.cxx
    ${SCINTILLA_ROOT}/src/ViewStyle.cxx
    ${SCINTILLA_ROOT}/src/XPM.cxx
    ${BOOSTREGEX_ROOT}/BoostRegExSearch.cxx
    ${BOOSTREGEX_ROOT}/UTF8DocumentIterator.cxx
)

add_library(npp-scintilla-qt STATIC ${NPP_SCINTILLA_SOURCES})
target_compile_definitions(npp-scintilla-qt
    PUBLIC SCINTILLA_QT MAKING_LIBRARY
    PRIVATE SCI_OWNREGEX SCI_NAMESPACE _CRT_SECURE_NO_DEPRECATE)
target_include_directories(npp-scintilla-qt
    PUBLIC
        "${SCINTILLA_ROOT}/include"
        "${SCINTILLA_QT_ROOT}"
        "${SCINTILLA_ROOT}/src"
    PRIVATE
        "${BOOSTREGEX_ROOT}"
)
target_link_libraries(npp-scintilla-qt PUBLIC Qt5::Core Qt5::Gui Qt5::Widgets)

get_target_property(NPP_QMAKE_EXECUTABLE Qt5::qmake IMPORTED_LOCATION)

include_directories(
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/src/uchardet
    ${SCINTILLA_ROOT}/include
    ${SCINTILLA_QT_ROOT}
    ${LEXILLA_ROOT}/include
)

# TinyXml 使用 STL wstring (generic_string = std::wstring)
add_compile_definitions(TIXML_USE_STL UNICODE _UNICODE)

# 创建输出目录
file(MAKE_DIRECTORY ${CMAKE_SOURCE_DIR}/src/MISC)
file(MAKE_DIRECTORY ${CMAKE_SOURCE_DIR}/src/WinControls/TabBar)
file(MAKE_DIRECTORY ${CMAKE_SOURCE_DIR}/src/WinControls/DockingWnd)
file(MAKE_DIRECTORY ${CMAKE_SOURCE_DIR}/src/WinControls/Preference)

file(GLOB UCHARDET_SOURCES CONFIGURE_DEPENDS
    ${CMAKE_SOURCE_DIR}/src/uchardet/*.cpp
)

# 源文件列表（将在后续添加）
set(SOURCES
    src/main.cpp
    src/CommandLineOptions.cpp
    src/NppCommandRegistry.cpp
    src/Parameters.cpp
    src/NativeLangSpeaker.cpp
    src/MainWindow.cpp
    src/TinyXml/tinyxml.cpp
    src/TinyXml/tinyxmlerror.cpp
    src/TinyXml/tinyxmlparser.cpp
    src/ScintillaComponent/ScintillaEditView.cpp
    src/ScintillaComponent/Buffer.cpp
    src/ScintillaComponent/FindReplaceDlg.cpp
    src/ScintillaComponent/ScintillaTextSearch.cpp
    src/ScintillaComponent/UserDefinedLexer.cpp
    src/ScintillaComponent/EditorMacro.cpp
    src/MISC/FileManager.cpp
    src/MISC/EncodingMapper.cpp
    src/MISC/TextFileCodec.cpp
    src/MISC/PlatformServices.cpp
    src/MISC/FileAssociationModel.cpp
    src/MISC/ConfigPathResolver.cpp
    src/MISC/ClosedFileHistory.cpp
    src/MISC/AutoCompletionParser.cpp
    src/MISC/FunctionListParser.cpp
    src/MISC/ToolbarIconTheme.cpp
    src/ScintillaComponent/Printer.cpp
    src/WinControls/TabBar/DocTabView.cpp
    src/WinControls/DockingWnd/FileBrowserPanel.cpp
    src/WinControls/DockingWnd/DockingManager.cpp
    src/WinControls/Preference/PreferenceDlg.cpp
    src/WinControls/DockingWnd/DocumentMapPanel.cpp
    src/WinControls/DockingWnd/FunctionListPanel.cpp
    src/WinControls/Grid/ShortcutMapper.cpp
    src/WinControls/ProjectPanel/WorkspaceDocument.cpp
    src/WinControls/ProjectPanel/ProjectPanel.cpp
    src/MISC/PluginsManager/PluginArtifactResolver.cpp
    src/MISC/PluginsManager/PluginCatalog.cpp
    src/WinControls/PluginsAdmin/PluginAdminModel.cpp
    src/WinControls/PluginsAdmin/PluginAdminDialog.cpp
    src/MISC/PluginsManager/PluginUpdatePlan.cpp
    src/MISC/PluginsManager/PluginLoadJournal.cpp
    src/MISC/PluginsManager/PluginManager.cpp
)

set(HEADERS
    src/Parameters.h
    src/CommandLineOptions.h
    src/NppCommandRegistry.h
    src/NativeLangSpeaker.h
    src/MainWindow.h
    src/ScintillaComponent/ScintillaEditView.h
    src/ScintillaComponent/Buffer.h
    src/ScintillaComponent/FindReplaceDlg.h
    src/ScintillaComponent/ScintillaTextSearch.h
    src/ScintillaComponent/UserDefinedLexer.h
    src/ScintillaComponent/EditorMacro.h
    src/MISC/FileManager.h
    src/MISC/EncodingMapper.h
    src/MISC/TextFileCodec.h
    src/MISC/PlatformServices.h
    src/MISC/FileAssociationModel.h
    src/MISC/ConfigPathResolver.h
    src/MISC/ClosedFileHistory.h
    src/MISC/AutoCompletionParser.h
    src/MISC/FunctionListParser.h
    src/MISC/ToolbarIconTheme.h
    src/ScintillaComponent/Printer.h
    src/WinControls/TabBar/DocTabView.h
    src/WinControls/DockingWnd/FileBrowserPanel.h
    src/WinControls/DockingWnd/DockingManager.h
    src/WinControls/Preference/PreferenceDlg.h
    src/WinControls/DockingWnd/DocumentMapPanel.h
    src/WinControls/DockingWnd/FunctionListPanel.h
    src/WinControls/Grid/ShortcutMapper.h
    src/WinControls/ProjectPanel/WorkspaceDocument.h
    src/WinControls/ProjectPanel/ProjectPanel.h
    src/MISC/PluginsManager/PluginArtifactResolver.h
    src/MISC/PluginsManager/PluginCatalog.h
    src/WinControls/PluginsAdmin/PluginAdminModel.h
    src/WinControls/PluginsAdmin/PluginAdminDialog.h
    src/MISC/PluginsManager/PluginUpdatePlan.h
    src/MISC/PluginsManager/PluginLoadJournal.h
    src/MISC/PluginsManager/IPlugin.h
    src/MISC/PluginsManager/PluginManager.h
)

if(WIN32)
    add_subdirectory(src/Win32PluginSystem)
    find_package(ZLIB QUIET)
    if(ZLIB_FOUND)
        set(NPP_ZLIB_TARGET ZLIB::ZLIB)
    else()
        get_filename_component(NPP_MINGW_BIN_DIR "${CMAKE_CXX_COMPILER}" DIRECTORY)
        get_filename_component(NPP_MINGW_ROOT "${NPP_MINGW_BIN_DIR}" DIRECTORY)
        find_path(NPP_ZLIB_INCLUDE_DIR zlib.h
            PATHS
                "${NPP_MINGW_ROOT}/x86_64-w64-mingw32/include"
                "${NPP_MINGW_ROOT}/i686-w64-mingw32/include"
            NO_DEFAULT_PATH)
        find_library(NPP_ZLIB_LIBRARY z
            PATHS
                "${NPP_MINGW_ROOT}/x86_64-w64-mingw32/lib"
                "${NPP_MINGW_ROOT}/i686-w64-mingw32/lib"
            NO_DEFAULT_PATH)
        if(NOT NPP_ZLIB_INCLUDE_DIR OR NOT NPP_ZLIB_LIBRARY)
            message(FATAL_ERROR "A zlib development library is required for Win32 MIME Tools SAML support")
        endif()
        add_library(npp-plugin-zlib STATIC IMPORTED)
        set_target_properties(npp-plugin-zlib PROPERTIES
            IMPORTED_LOCATION "${NPP_ZLIB_LIBRARY}"
            INTERFACE_INCLUDE_DIRECTORIES "${NPP_ZLIB_INCLUDE_DIR}")
        set(NPP_ZLIB_TARGET npp-plugin-zlib)
    endif()
endif()

# 资源文件
set(RESOURCES
    resources/resources.qrc
)

add_executable(notepadpp-qt WIN32
    ${SOURCES} ${HEADERS} ${RESOURCES} ${UCHARDET_SOURCES})

target_link_libraries(notepadpp-qt
    npp-scintilla-qt
    npp-lexilla
    Qt5::Core
    Qt5::Gui
    Qt5::Widgets
    Qt5::Network
    Qt5::PrintSupport
    Qt5::Xml
)
if(WIN32)
    target_link_libraries(notepadpp-qt ${NPP_ZLIB_TARGET})
    foreach(qt_runtime Core Gui Widgets Network PrintSupport Xml)
        add_custom_command(TARGET notepadpp-qt POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "$<TARGET_FILE:Qt5::${qt_runtime}>"
                "$<TARGET_FILE_DIR:notepadpp-qt>"
        )
    endforeach()

    get_filename_component(NPP_QT_BIN_DIR "${NPP_QMAKE_EXECUTABLE}" DIRECTORY)
    get_filename_component(NPP_QT_PLUGIN_DIR
        "${NPP_QT_BIN_DIR}/../plugins" ABSOLUTE)
    foreach(qt_plugin_dir platforms imageformats)
        if(EXISTS "${NPP_QT_PLUGIN_DIR}/${qt_plugin_dir}")
            add_custom_command(TARGET notepadpp-qt POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_directory
                    "${NPP_QT_PLUGIN_DIR}/${qt_plugin_dir}"
                    "$<TARGET_FILE_DIR:notepadpp-qt>/${qt_plugin_dir}"
            )
        endif()
    endforeach()

    if(MINGW)
        file(GLOB NPP_MINGW_RUNTIME_DLLS
            "${NPP_COMPILER_BIN}/libgcc_s_*-1.dll"
            "${NPP_COMPILER_BIN}/libstdc++-6.dll"
            "${NPP_COMPILER_BIN}/libwinpthread-1.dll")
        foreach(runtime_dll IN LISTS NPP_MINGW_RUNTIME_DLLS)
            add_custom_command(TARGET notepadpp-qt POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    "${runtime_dll}" "$<TARGET_FILE_DIR:notepadpp-qt>"
            )
        endforeach()
    endif()
endif()
add_custom_command(TARGET notepadpp-qt POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_SOURCE_DIR}/resources/functionList"
        "$<TARGET_FILE_DIR:notepadpp-qt>/functionList"
)

if(WIN32)
    target_link_libraries(notepadpp-qt shell32 advapi32 version comctl32)
endif()

add_executable(npp-plugin-updater
    src/MISC/PluginsManager/updater/PluginUpdaterMain.cpp
    src/MISC/PluginsManager/updater/PluginArchiveExtractor.cpp
    src/MISC/PluginsManager/PluginArtifactResolver.cpp
    src/MISC/PluginsManager/updater/PluginUpdateExecutor.cpp
    src/MISC/PluginsManager/PluginUpdatePlan.cpp
)
target_link_libraries(npp-plugin-updater Qt5::Core Qt5::Network)
if(WIN32)
    target_link_libraries(npp-plugin-updater advapi32)
endif()
add_dependencies(notepadpp-qt npp-plugin-updater)
add_custom_command(TARGET notepadpp-qt POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "$<TARGET_FILE:npp-plugin-updater>"
        "$<TARGET_FILE_DIR:notepadpp-qt>"
)

# 如果启用插件系统
if(ENABLE_PLUGIN_SYSTEM)
    target_compile_definitions(notepadpp-qt PRIVATE ENABLE_PLUGIN_SYSTEM)
    message(STATUS "Plugin system: ENABLED")
else()
    message(STATUS "Plugin system: DISABLED (use -DENABLE_PLUGIN_SYSTEM=ON to enable)")
endif()

if(BUILD_TESTING)
    add_executable(npp-command-registry-tests
        tests/NppCommandRegistryTests.cpp
        src/NppCommandRegistry.cpp
    )
    target_compile_definitions(npp-command-registry-tests PRIVATE
        NPP_MAINWINDOW_SOURCE="${CMAKE_SOURCE_DIR}/src/MainWindow.cpp")
    target_link_libraries(npp-command-registry-tests Qt5::Core)
    add_test(NAME npp-command-registry-tests COMMAND npp-command-registry-tests)

    add_executable(file-association-model-tests
        tests/FileAssociationModelTests.cpp
        src/MISC/FileAssociationModel.cpp
    )
    target_link_libraries(file-association-model-tests Qt5::Core)
    add_test(NAME file-association-model-tests COMMAND file-association-model-tests)

    add_executable(toolbar-icon-theme-tests
        tests/ToolbarIconThemeTests.cpp
        src/MISC/ToolbarIconTheme.cpp
    )
    target_compile_definitions(toolbar-icon-theme-tests PRIVATE
        NPP_TEST_ICON_SOURCE="${CMAKE_SOURCE_DIR}/resources/icons/userDefineDlg_off.ico")
    target_link_libraries(toolbar-icon-theme-tests Qt5::Core Qt5::Gui)
    add_test(NAME toolbar-icon-theme-tests COMMAND toolbar-icon-theme-tests)
    if(WIN32)
        set_tests_properties(toolbar-icon-theme-tests PROPERTIES
            ENVIRONMENT "QT_QPA_PLATFORM=windows")
    elseif(UNIX AND NOT APPLE)
        set_tests_properties(toolbar-icon-theme-tests PROPERTIES
            ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
    endif()

    if(WIN32)
        add_executable(file-association-runtime-test
            tests/FileAssociationRuntimeTest.cpp
            src/MISC/FileAssociationModel.cpp
            src/MISC/PlatformServices.cpp
        )
        target_link_libraries(file-association-runtime-test
            Qt5::Core Qt5::Gui shell32 advapi32)
    endif()

    add_executable(command-line-options-tests
        tests/CommandLineOptionsTests.cpp
        src/CommandLineOptions.cpp
    )
    target_link_libraries(command-line-options-tests Qt5::Core)
    add_test(NAME command-line-options-tests COMMAND command-line-options-tests)

    add_executable(plugin-admin-tests
        tests/PluginAdminTests.cpp
        src/MISC/PluginsManager/PluginArtifactResolver.cpp
        src/MISC/PluginsManager/PluginCatalog.cpp
        src/WinControls/PluginsAdmin/PluginAdminModel.cpp
        src/MISC/PluginsManager/PluginUpdatePlan.cpp
        ${RESOURCES}
    )
    target_link_libraries(plugin-admin-tests Qt5::Core)
    if(WIN32)
        target_link_libraries(plugin-admin-tests version)
    endif()
    add_test(NAME plugin-admin-tests COMMAND plugin-admin-tests)

    add_executable(plugin-updater-tests
        tests/PluginUpdaterTests.cpp
        src/MISC/PluginsManager/updater/PluginArchiveExtractor.cpp
        src/MISC/PluginsManager/PluginArtifactResolver.cpp
        src/MISC/PluginsManager/updater/PluginUpdateExecutor.cpp
        src/MISC/PluginsManager/PluginUpdatePlan.cpp
    )
    target_link_libraries(plugin-updater-tests Qt5::Core Qt5::Network)
    target_compile_definitions(plugin-updater-tests PRIVATE
        NPP_PLUGIN_UPDATER_PATH="$<TARGET_FILE:npp-plugin-updater>")
    add_dependencies(plugin-updater-tests npp-plugin-updater)
    add_test(NAME plugin-updater-tests COMMAND plugin-updater-tests)

    add_executable(plugin-load-journal-tests
        tests/PluginLoadJournalTests.cpp
        src/MISC/PluginsManager/PluginLoadJournal.cpp
    )
    target_link_libraries(plugin-load-journal-tests Qt5::Core)
    add_test(NAME plugin-load-journal-tests
        COMMAND plugin-load-journal-tests)

    add_executable(plugin-investigation-tests
        tests/PluginInvestigationTests.cpp
    )
    target_compile_definitions(plugin-investigation-tests PRIVATE
        NPP_PLUGIN_CATALOG="${CMAKE_SOURCE_DIR}/resources/pluginList/windows/pl.x86.json"
        NPP_PLUGIN_INVENTORY="${CMAKE_SOURCE_DIR}/codex/analysis/plugins-v846/inventory.md"
        NPP_PLUGIN_IMPORTANCE="${CMAKE_SOURCE_DIR}/codex/analysis/plugins-v846/importance.md"
        NPP_PLUGIN_BATCH_A_C="${CMAKE_SOURCE_DIR}/codex/analysis/plugins-v846/batch-a-c.md"
        NPP_PLUGIN_BATCH_D_H="${CMAKE_SOURCE_DIR}/codex/analysis/plugins-v846/batch-d-h.md"
        NPP_PLUGIN_BATCH_I_M="${CMAKE_SOURCE_DIR}/codex/analysis/plugins-v846/batch-i-m.md"
        NPP_PLUGIN_BATCH_N="${CMAKE_SOURCE_DIR}/codex/analysis/plugins-v846/batch-n.md"
        NPP_PLUGIN_BATCH_O_S="${CMAKE_SOURCE_DIR}/codex/analysis/plugins-v846/batch-o-s.md"
        NPP_PLUGIN_BATCH_T_Z="${CMAKE_SOURCE_DIR}/codex/analysis/plugins-v846/batch-t-z.md"
        NPP_PLUGIN_API_MATRIX="${CMAKE_SOURCE_DIR}/codex/analysis/plugins-v846/api-dependency-matrix.md"
    )
    target_link_libraries(plugin-investigation-tests Qt5::Core)
    add_test(NAME plugin-investigation-tests
        COMMAND plugin-investigation-tests)

    add_executable(config-path-resolver-tests
        tests/ConfigPathResolverTests.cpp
        src/MISC/ConfigPathResolver.cpp
    )
    target_link_libraries(config-path-resolver-tests Qt5::Core)
    add_test(NAME config-path-resolver-tests COMMAND config-path-resolver-tests)

    add_executable(closed-file-history-tests
        tests/ClosedFileHistoryTests.cpp
        src/MISC/ClosedFileHistory.cpp
    )
    target_link_libraries(closed-file-history-tests Qt5::Core)
    add_test(NAME closed-file-history-tests COMMAND closed-file-history-tests)

    add_executable(auto-completion-parser-tests
        tests/AutoCompletionParserTests.cpp
        src/MISC/AutoCompletionParser.cpp
    )
    target_link_libraries(auto-completion-parser-tests Qt5::Core)
    add_test(NAME auto-completion-parser-tests
        COMMAND auto-completion-parser-tests)

    add_executable(parameters-style-udl-tests
        tests/ParametersStyleUdlTests.cpp
        src/Parameters.cpp
        src/MISC/ConfigPathResolver.cpp
        src/NativeLangSpeaker.cpp
        src/TinyXml/tinyxml.cpp
        src/TinyXml/tinyxmlerror.cpp
        src/TinyXml/tinyxmlparser.cpp
        resources/resources.qrc
    )
    target_compile_definitions(parameters-style-udl-tests PRIVATE
        NPP_TEST_STYLERS_SOURCE="${CMAKE_SOURCE_DIR}/resources/stylers.model.xml"
        NPP_TEST_UDL_SOURCE="${CMAKE_SOURCE_DIR}/resources/userDefineLang.xml")
    target_link_libraries(parameters-style-udl-tests
        Qt5::Core Qt5::Gui Qt5::Widgets Qt5::Xml)
    add_test(NAME parameters-style-udl-tests
        COMMAND parameters-style-udl-tests)
    if(UNIX AND NOT APPLE)
        set_tests_properties(parameters-style-udl-tests PROPERTIES
            ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
    endif()

    add_executable(function-list-parser-tests
        tests/FunctionListParserTests.cpp
        src/MISC/FunctionListParser.cpp
    )
    target_compile_definitions(function-list-parser-tests PRIVATE
        NPP_FUNCTION_LIST_SOURCE="${CMAKE_SOURCE_DIR}/resources/functionList")
    target_link_libraries(function-list-parser-tests Qt5::Core Qt5::Xml)
    add_test(NAME function-list-parser-tests COMMAND function-list-parser-tests)

    add_executable(find-dialog-localization-tests
        tests/FindDialogLocalizationTests.cpp
    )
    target_compile_definitions(find-dialog-localization-tests PRIVATE
        NPP_MAINWINDOW_SOURCE="${CMAKE_SOURCE_DIR}/src/MainWindow.cpp"
        NPP_FIND_DIALOG_SOURCE="${CMAKE_SOURCE_DIR}/src/ScintillaComponent/FindReplaceDlg.cpp"
        NPP_PREFERENCES_SOURCE="${CMAKE_SOURCE_DIR}/src/WinControls/Preference/PreferenceDlg.cpp"
        NPP_FIND_LANGUAGE_SOURCE="${CMAKE_SOURCE_DIR}/resources/nativeLang/chineseSimplified.xml"
    )
    target_link_libraries(find-dialog-localization-tests Qt5::Core Qt5::Xml)
    add_test(NAME find-dialog-localization-tests
        COMMAND find-dialog-localization-tests)

    add_executable(docking-manager-tests
        tests/DockingManagerTests.cpp
        src/WinControls/DockingWnd/DockingManager.cpp
        src/WinControls/DockingWnd/DockingManager.h
    )
    target_include_directories(docking-manager-tests PRIVATE
        "${CMAKE_SOURCE_DIR}/src")
    target_link_libraries(docking-manager-tests Qt5::Core Qt5::Gui Qt5::Widgets)
    add_test(NAME docking-manager-tests COMMAND docking-manager-tests)
    if(WIN32)
        set_tests_properties(docking-manager-tests PROPERTIES
            ENVIRONMENT "QT_QPA_PLATFORM=windows")
    elseif(UNIX AND NOT APPLE)
        set_tests_properties(docking-manager-tests PROPERTIES
            ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
    endif()

    set(UI_PARITY_SOURCES ${SOURCES})
    list(REMOVE_ITEM UI_PARITY_SOURCES src/main.cpp)
    if(WIN32)
        add_library(win32-invalid-registration-plugin SHARED
            tests/plugins/Win32InvalidRegistrationPlugin.cpp)
        add_library(win32-valid-registration-plugin SHARED
            tests/plugins/Win32ValidRegistrationPlugin.cpp)
        foreach(plugin_target IN ITEMS
                win32-invalid-registration-plugin
                win32-valid-registration-plugin)
            target_include_directories(${plugin_target} PRIVATE
                "${CMAKE_SOURCE_DIR}/src"
                "${NPP_SCINTILLA_SOURCE_DIR}/include")
            set_target_properties(${plugin_target} PROPERTIES PREFIX "")
        endforeach()
    endif()
    add_executable(ui-parity-capture
        tests/UiParityCapture.cpp
        ${UI_PARITY_SOURCES}
        ${HEADERS}
        ${RESOURCES}
        ${UCHARDET_SOURCES}
    )
    target_link_libraries(ui-parity-capture
        npp-scintilla-qt
        npp-lexilla
        Qt5::Core
        Qt5::Gui
        Qt5::Widgets
        Qt5::Network
        Qt5::PrintSupport
        Qt5::Xml
    )
    set(NPP_MIMETOOLS_MANAGED_FIXTURE FALSE)
    if(WIN32)
        target_link_libraries(ui-parity-capture
            shell32 advapi32 version comctl32 ${NPP_ZLIB_TARGET})
        add_dependencies(ui-parity-capture
            win32-invalid-registration-plugin
            win32-valid-registration-plugin)
        target_compile_definitions(ui-parity-capture PRIVATE
            NPP_PLUGIN_REGISTRATION_TEST=1
            NPP_INVALID_REGISTRATION_PLUGIN="$<TARGET_FILE:win32-invalid-registration-plugin>"
            NPP_VALID_REGISTRATION_PLUGIN="$<TARGET_FILE:win32-valid-registration-plugin>")
        if(CMAKE_SIZEOF_VOID_P EQUAL 8 AND
           NOT CMAKE_SYSTEM_PROCESSOR MATCHES "ARM64|aarch64")
            set(NPP_MIMETOOLS_ARCHIVE
                "${CMAKE_SOURCE_DIR}/third_party/win32-plugins/mimeTools/2.8/x64/mimetools.v2.8.x64.zip")
            if(NOT EXISTS "${NPP_MIMETOOLS_ARCHIVE}")
                message(FATAL_ERROR "The mimeTools test archive is missing")
            endif()
            file(SHA256 "${NPP_MIMETOOLS_ARCHIVE}"
                 NPP_MIMETOOLS_ARCHIVE_SHA256)
            if(NOT NPP_MIMETOOLS_ARCHIVE_SHA256 STREQUAL
                   "ed5133f8a0552e974135ada78a0260581be979f916ddbcd45697d2a1a1b8f280")
                message(FATAL_ERROR "The mimeTools test archive hash is invalid")
            endif()
            file(TO_CMAKE_PATH "${NPP_MIMETOOLS_ARCHIVE}"
                 NPP_MIMETOOLS_ARCHIVE_URL_PATH)
            function(npp_prepare_plugin_archive variable relative_path
                     expected_sha256)
                set(NPP_PLUGIN_ARCHIVE_PATH
                    "${CMAKE_SOURCE_DIR}/${relative_path}")
                if(NOT EXISTS "${NPP_PLUGIN_ARCHIVE_PATH}")
                    message(FATAL_ERROR
                        "The ${variable} test archive is missing")
                endif()
                file(SHA256 "${NPP_PLUGIN_ARCHIVE_PATH}"
                    NPP_PLUGIN_ARCHIVE_SHA256)
                if(NOT NPP_PLUGIN_ARCHIVE_SHA256 STREQUAL
                       expected_sha256)
                    message(FATAL_ERROR
                        "The ${variable} test archive hash is invalid")
                endif()
                file(TO_CMAKE_PATH "${NPP_PLUGIN_ARCHIVE_PATH}"
                    NPP_PLUGIN_ARCHIVE_URL_PATH)
                set(${variable}_URL_PATH "${NPP_PLUGIN_ARCHIVE_URL_PATH}"
                    PARENT_SCOPE)
            endfunction()
            npp_prepare_plugin_archive(NPP_REVERSE_LINES_ARCHIVE
                "third_party/win32-plugins/qkNppReverseLines/1.0.0.0/x64/qkNppReverseLinesPlugin_v1.0.0.0_npp7.7_x64.zip"
                "d4b4d9aa51080bf1e829ca124ab5484247a034524f625c55b8d2c261676352bb")
            npp_prepare_plugin_archive(NPP_REMOVE_DUPLICATE_LINES_ARCHIVE
                "third_party/win32-plugins/RemoveDuplicateLines/1.3.0.0/x64/Remove_dup_lines_x64.zip"
                "f90bf1803e703c0a10cfa9fb022e914628e2abd4c23e666e6b9a4e11bcf0464f")
            npp_prepare_plugin_archive(NPP_SELECT_QUOTED_TEXT_ARCHIVE
                "third_party/win32-plugins/SelectQuotedText/1.0.0/x64/SelectQuotedText-100-x64.zip"
                "3e53432d770d39c8d9d004fdc69512764cb82f2468e6f4b9cb61ac51fb618b04")
            npp_prepare_plugin_archive(NPP_BRACKETS_CHECK_ARCHIVE
                "third_party/win32-plugins/BracketsCheck/1.2.2/x64/BracketsCheck_1-2-2_x64.zip"
                "a56363d21f392e0e67b4110555e1d2a0477e55ac8368abccc5c0f9f2d11f5ba9")
            npp_prepare_plugin_archive(NPP_POOR_MANS_TSQL_ARCHIVE
                "third_party/win32-plugins/PoorMansTSqlFormatterNppPlugin/1.6.13.31508/x64/SqlFormatterNppPlugin.x64.1.6.13.zip"
                "ea99ec04ba73f027b92f086d2345efb544ef62a922e4755fab7f875b49803bb2")
            npp_prepare_plugin_archive(NPP_SECURE_PAD_ARCHIVE
                "third_party/win32-plugins/SecurePad/2.4/x64/SecurePad_v2.4_x64.zip"
                "4e565cb4417ad2ff5c5db19dd6f0bacdf90a8bf268c9f54d1693e9c50526ffa8")
            npp_prepare_plugin_archive(NPP_CODE_ALIGNMENT_ARCHIVE
                "third_party/win32-plugins/CodeAlignmentNpp/14.1.107/x64/CodeAlignmentNpp_v14.1_x64.zip"
                "a11c60ad2dcaf0064a5c3cca01aa2d2a6743897875e23887867783a2756b796b")
            npp_prepare_plugin_archive(NPP_BETTER_MULTI_SELECTION_ARCHIVE
                "third_party/win32-plugins/BetterMultiSelection/1.5/x64/BetterMultiSelection_v1.5_x64.zip"
                "a33dddd93d40a2bb595ea44eeb5bcdaefd3189c5fae3c40a052da9d02597bb63")
            npp_prepare_plugin_archive(NPP_JSON_VIEWER_ARCHIVE
                "third_party/win32-plugins/NPPJSONViewer/1.41/x64/NPPJSONViewer_x64.zip"
                "f9e3f6e1d93e088f7477cd6bc3cb9767b803acc4171533155435fbc02ae8926a")
            npp_prepare_plugin_archive(NPP_JSON_TOOLS_ARCHIVE
                "third_party/win32-plugins/JsonTools/3.2.0/x64/Release_x64.zip"
                "884704a9dc046616831a8df6013114100472e4a7b7a4bb62e40309b286b33a0d")
            npp_prepare_plugin_archive(NPP_CONVERTER_ARCHIVE
                "third_party/win32-plugins/nppConverter/4.4.0/x64/nppConvert.v4.4.x64.zip"
                "e7eafbac35d91572aa93c5fd6a8a957669b50d0160aa2e35e9e632e2ac6ab63d")
            npp_prepare_plugin_archive(NPP_PLUGIN_DEMO_ARCHIVE
                "third_party/win32-plugins/NppPluginDemo/4.2/x64/pluginDemo.v4.2.bin.x64.zip"
                "afa33fe958a907c2b80c0e8ae26deaea533455a2a2826f30a9b46fba0ba59941")
            npp_prepare_plugin_archive(NPP_GOTO_LINE_COL_ARCHIVE
                "third_party/win32-plugins/GotoLineCol/2.4.2.0/x64/GotoLineCol_x64.zip"
                "afaa08a53ec8482875454cb1270ab4faf870e2fcefbed9c4e628ae0058920a12")
            npp_prepare_plugin_archive(NPP_RANDOM_VALUES_ARCHIVE
                "third_party/win32-plugins/RandomValuesNppPlugin/0.2.1/x64/RandomValuesNppPlugin_x64.zip"
                "b404204c47e5eccf621701c529499bf989b8a8adfb69a6d2cdfbcb5ff60ab3be")
            npp_prepare_plugin_archive(NPP_MERGE_FILES_ARCHIVE
                "third_party/win32-plugins/Merge files in one/1.2.0.0/x64/Merge.files.in.one_x64.zip"
                "eab174d0f5926a64a75978c7efe2ff508c9bb7c13f7c97eb8bb410afa8604fed")
            npp_prepare_plugin_archive(NPP_SELECT_TO_CLIPBOARD_ARCHIVE
                "third_party/win32-plugins/SelectToClipboard/1.0.3/x64/SelectToClipboard_x64_v1.0.3.zip"
                "b5dfc0dbfea79cb609d7f6a522e824a1b3ea9ecc471c215eaf28a846906e93c5")
            npp_prepare_plugin_archive(NPP_URL_PLUGIN_ARCHIVE
                "third_party/win32-plugins/urlPlugin/1.2.0.0/x64/urlPlugin_x64.zip"
                "3bab2b9494d6fbdaac17be5b410a18c074dec5e7dc440e2c1c5c61aaecb57588")
            file(TO_CMAKE_PATH "${CMAKE_BINARY_DIR}/notepadpp-qt.exe"
                 NPP_MIMETOOLS_TEST_APPLICATION)
            file(TO_CMAKE_PATH "${CMAKE_BINARY_DIR}/plugins"
                 NPP_MIMETOOLS_TEST_PLUGIN_ROOT)
            configure_file(
                tests/Win32PluginCorpusInstallPlan.json.in
                "${CMAKE_BINARY_DIR}/win32-plugin-corpus-ctest-install-plan.json"
                @ONLY)
            target_compile_definitions(ui-parity-capture PRIVATE
                NPP_MIMETOOLS_MANAGED_TEST=1
                NPP_WIN32_PLUGIN_CORPUS_MANAGED_TEST=1)
            add_test(NAME win32-plugin-corpus-managed-install
                COMMAND npp-plugin-updater
                    --plan "${CMAKE_BINARY_DIR}/win32-plugin-corpus-ctest-install-plan.json"
                    --no-restart)
            set_tests_properties(win32-plugin-corpus-managed-install PROPERTIES
                FIXTURES_SETUP mimetools-managed-plugin)
            set(NPP_MIMETOOLS_MANAGED_FIXTURE TRUE)
        endif()
    endif()
    add_test(NAME ui-localization-runtime-100
        COMMAND ui-parity-capture
            "${CMAKE_BINARY_DIR}/ui-localization-runtime-100"
            light zh_CN)
    add_test(NAME ui-localization-runtime-150
        COMMAND ui-parity-capture
            "${CMAKE_BINARY_DIR}/ui-localization-runtime-150"
            light zh_CN)
    add_test(NAME ui-localization-dark-runtime-100
        COMMAND ui-parity-capture
            "${CMAKE_BINARY_DIR}/ui-localization-dark-runtime-100"
            dark zh_CN)
    add_test(NAME ui-localization-dark-runtime-150
        COMMAND ui-parity-capture
            "${CMAKE_BINARY_DIR}/ui-localization-dark-runtime-150"
            dark zh_CN)
    add_test(NAME ui-no-plugin-runtime
        COMMAND ui-parity-capture
            "${CMAKE_BINARY_DIR}/ui-no-plugin-runtime"
            noPlugin)
    if(NPP_MIMETOOLS_MANAGED_FIXTURE)
        add_test(NAME ui-plugin-load-recovery
            COMMAND ui-parity-capture
                "${CMAKE_BINARY_DIR}/ui-plugin-load-recovery"
                pluginRecovery)
    endif()
    if(WIN32)
        add_test(NAME ui-plugin-registration-rollback
            COMMAND ui-parity-capture
                "${CMAKE_BINARY_DIR}/ui-plugin-registration-rollback"
                registrationRollback)
    endif()
    if(WIN32)
        set(UI_TEST_PLATFORM "QT_QPA_PLATFORM=windows")
    elseif(UNIX AND NOT APPLE)
        set(UI_TEST_PLATFORM "QT_QPA_PLATFORM=offscreen")
    else()
        set(UI_TEST_PLATFORM "")
    endif()
    set_tests_properties(ui-localization-runtime-100 PROPERTIES
        ENVIRONMENT "QT_SCALE_FACTOR=1.0;${UI_TEST_PLATFORM}")
    set_tests_properties(ui-localization-runtime-150 PROPERTIES
        ENVIRONMENT "QT_SCALE_FACTOR=1.5;${UI_TEST_PLATFORM}")
    set_tests_properties(ui-localization-dark-runtime-100 PROPERTIES
        ENVIRONMENT "QT_SCALE_FACTOR=1.0;${UI_TEST_PLATFORM}")
    set_tests_properties(ui-localization-dark-runtime-150 PROPERTIES
        ENVIRONMENT "QT_SCALE_FACTOR=1.5;${UI_TEST_PLATFORM}")
    set_tests_properties(ui-no-plugin-runtime PROPERTIES
        ENVIRONMENT "QT_SCALE_FACTOR=1.0;${UI_TEST_PLATFORM}")
    if(NPP_MIMETOOLS_MANAGED_FIXTURE)
        set_tests_properties(ui-plugin-load-recovery PROPERTIES
            ENVIRONMENT "QT_SCALE_FACTOR=1.0;${UI_TEST_PLATFORM}")
    endif()
    if(WIN32)
        set_tests_properties(ui-plugin-registration-rollback PROPERTIES
            ENVIRONMENT "QT_SCALE_FACTOR=1.0;${UI_TEST_PLATFORM}")
    endif()
    if(NPP_MIMETOOLS_MANAGED_FIXTURE)
        set_tests_properties(
            ui-localization-runtime-100
            ui-localization-runtime-150
            ui-localization-dark-runtime-100
            ui-localization-dark-runtime-150
            ui-plugin-load-recovery
            PROPERTIES FIXTURES_REQUIRED mimetools-managed-plugin)
    endif()

    add_executable(p1-runtime-capture
        tests/P1RuntimeCapture.cpp
        ${UI_PARITY_SOURCES}
        ${HEADERS}
        ${RESOURCES}
        ${UCHARDET_SOURCES}
    )
    target_link_libraries(p1-runtime-capture
        npp-scintilla-qt
        npp-lexilla
        Qt5::Core
        Qt5::Gui
        Qt5::Widgets
        Qt5::Network
        Qt5::PrintSupport
        Qt5::Xml
    )
    if(WIN32)
        target_link_libraries(p1-runtime-capture
            ${NPP_ZLIB_TARGET}
            shell32
            advapi32
            version
            comctl32
        )
    endif()

    add_executable(core-behavior-tests
        tests/CoreBehaviorTests.cpp
        src/MISC/EncodingMapper.cpp
        src/MISC/TextFileCodec.cpp
        src/ScintillaComponent/Buffer.cpp
        ${UCHARDET_SOURCES}
    )
    target_link_libraries(core-behavior-tests Qt5::Core)
    add_test(NAME core-behavior-tests COMMAND core-behavior-tests)

    add_executable(workspace-document-tests
        tests/WorkspaceDocumentTests.cpp
        src/WinControls/ProjectPanel/WorkspaceDocument.cpp
    )
    target_link_libraries(workspace-document-tests Qt5::Core)
    add_test(NAME workspace-document-tests COMMAND workspace-document-tests)

    add_executable(lexilla-integration-tests
        tests/LexillaIntegrationTests.cpp)
    target_link_libraries(lexilla-integration-tests npp-lexilla)
    add_test(NAME lexilla-integration-tests
        COMMAND lexilla-integration-tests)

    add_executable(large-file-mode-tests
        tests/LargeFileModeTests.cpp
        src/Parameters.cpp
        src/MISC/ConfigPathResolver.cpp
        src/NativeLangSpeaker.cpp
        src/TinyXml/tinyxml.cpp
        src/TinyXml/tinyxmlerror.cpp
        src/TinyXml/tinyxmlparser.cpp
        src/ScintillaComponent/ScintillaEditView.cpp
        src/ScintillaComponent/Buffer.cpp
        src/ScintillaComponent/UserDefinedLexer.cpp
        src/MISC/FileManager.cpp
        src/MISC/EncodingMapper.cpp
        src/MISC/TextFileCodec.cpp
        src/MISC/AutoCompletionParser.cpp
        resources/resources.qrc
        ${UCHARDET_SOURCES}
    )
    target_link_libraries(large-file-mode-tests
        npp-scintilla-qt
        npp-lexilla
        Qt5::Core
        Qt5::Gui
        Qt5::Widgets
        Qt5::PrintSupport
    )
    target_include_directories(large-file-mode-tests PRIVATE
        "${SCINTILLA_ROOT}/include")
    add_test(NAME large-file-mode-tests COMMAND large-file-mode-tests)
    if(WIN32)
        set_tests_properties(large-file-mode-tests PROPERTIES
            ENVIRONMENT "QT_QPA_PLATFORM=windows")
    elseif(UNIX AND NOT APPLE)
        set_tests_properties(large-file-mode-tests PROPERTIES
            ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
    endif()

    add_executable(boost-regex-tests
        tests/BoostRegexTests.cpp
        src/ScintillaComponent/ScintillaTextSearch.cpp
    )
    target_link_libraries(boost-regex-tests
        npp-scintilla-qt
        npp-lexilla
        Qt5::Widgets
        Qt5::PrintSupport
    )
    add_test(NAME boost-regex-tests COMMAND boost-regex-tests)
    if(WIN32)
        set_tests_properties(boost-regex-tests PROPERTIES
            ENVIRONMENT "QT_QPA_PLATFORM=windows")
    elseif(UNIX AND NOT APPLE)
        set_tests_properties(boost-regex-tests PROPERTIES
            ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
    endif()

    add_executable(config-corpus-tests
        tests/ConfigCorpusTests.cpp
        src/Parameters.cpp
        src/MISC/ConfigPathResolver.cpp
        src/NativeLangSpeaker.cpp
        src/TinyXml/tinyxml.cpp
        src/TinyXml/tinyxmlerror.cpp
        src/TinyXml/tinyxmlparser.cpp
        resources/resources.qrc
    )
    target_link_libraries(config-corpus-tests
        Qt5::Core
        Qt5::Gui
        Qt5::Widgets
        Qt5::Xml
    )
    add_custom_command(TARGET config-corpus-tests POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "$<TARGET_FILE:Qt5::Xml>"
            "$<TARGET_FILE_DIR:config-corpus-tests>"
    )

    function(add_config_corpus_test test_name mode corpus_path)
        if(EXISTS "${corpus_path}")
            add_test(NAME "${test_name}"
                COMMAND config-corpus-tests "${mode}" "${corpus_path}")
            if(UNIX AND NOT APPLE)
                set_tests_properties("${test_name}" PROPERTIES
                    ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
            endif()
        endif()
    endfunction()

    set(NPP_V846_BIN "${CMAKE_SOURCE_DIR}/tests/corpus/v8.4.6")
    set(NPP_V846_SRC "${CMAKE_SOURCE_DIR}/tests/corpus/v8.4.6")
    add_config_corpus_test(config-v846 config "${NPP_V846_BIN}/config.xml")
    add_config_corpus_test(shortcuts-v846 shortcuts
        "${NPP_V846_BIN}/shortcuts.xml")
    add_config_corpus_test(langs-v846 langs "${NPP_V846_BIN}/langs.xml")
    add_config_corpus_test(stylers-v846 stylers "${NPP_V846_BIN}/stylers.xml")
    add_config_corpus_test(context-v846 context
        "${NPP_V846_BIN}/contextMenu.xml")
    add_config_corpus_test(udl-v846 udl
        "${NPP_V846_SRC}/userDefineLang.xml")

    file(TO_CMAKE_PATH "$ENV{APPDATA}/Notepad++" NPP_REAL_USER_CONFIG)
    add_config_corpus_test(config-current-user config
        "${NPP_REAL_USER_CONFIG}/config.xml")
    add_config_corpus_test(session-current-user session
        "${NPP_REAL_USER_CONFIG}/session.xml")
    add_config_corpus_test(shortcuts-current-user shortcuts
        "${NPP_REAL_USER_CONFIG}/shortcuts.xml")

endif()
