cmake_minimum_required(VERSION 3.10)
project(NotepadPlusPlusQt VERSION 8.4.6 LANGUAGES CXX)

include(CTest)

set(CMAKE_CXX_STANDARD 14)
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

# Bundled QScintilla source used by the Notepad++ static editor core.
set(QSCINTILLA_ROOT "${CMAKE_SOURCE_DIR}/third_party/qscintilla")
set(QSCINTILLA_INCLUDE_DIR "${QSCINTILLA_ROOT}/src")
get_target_property(NPP_QMAKE_EXECUTABLE Qt5::qmake IMPORTED_LOCATION)
if(NOT NPP_QMAKE_EXECUTABLE)
    find_program(NPP_QMAKE_EXECUTABLE NAMES qmake qmake-qt5 REQUIRED)
endif()

if(MINGW)
    get_filename_component(_qt_bin_dir "${NPP_QMAKE_EXECUTABLE}" DIRECTORY)
    get_filename_component(_qt_prefix_dir "${_qt_bin_dir}/../../.." ABSOLUTE)
    find_program(NPP_QMAKE_BUILD_TOOL NAMES mingw32-make
        HINTS "${_qt_prefix_dir}/Tools/mingw730_64/bin")
elseif(MSVC)
    find_program(NPP_QMAKE_BUILD_TOOL NAMES nmake jom REQUIRED)
else()
    find_program(NPP_QMAKE_BUILD_TOOL NAMES make gmake REQUIRED)
endif()
if(NOT NPP_QMAKE_BUILD_TOOL)
    message(FATAL_ERROR "Could not find the build tool required by qmake")
endif()

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(NPP_QSCINTILLA_CONFIG debug)
else()
    set(NPP_QSCINTILLA_CONFIG release)
endif()

set(NPP_QSCINTILLA_BUILD_DIR
    "${CMAKE_BINARY_DIR}/qscintilla-npp-local-${NPP_QSCINTILLA_CONFIG}")
set(NPP_QSCINTILLA_ARCHIVE
    "${NPP_QSCINTILLA_BUILD_DIR}/stage/${CMAKE_STATIC_LIBRARY_PREFIX}qscintilla2_qt5_npp${CMAKE_STATIC_LIBRARY_SUFFIX}")
get_filename_component(NPP_COMPILER_BIN "${CMAKE_CXX_COMPILER}" DIRECTORY)

add_custom_command(
    OUTPUT "${NPP_QSCINTILLA_ARCHIVE}"
    COMMAND "${CMAKE_COMMAND}"
        "-DNPP_QMAKE_EXECUTABLE=${NPP_QMAKE_EXECUTABLE}"
        "-DNPP_QMAKE_BUILD_TOOL=${NPP_QMAKE_BUILD_TOOL}"
        "-DNPP_COMPILER_BIN=${NPP_COMPILER_BIN}"
        "-DNPP_QSCINTILLA_PROJECT=${QSCINTILLA_ROOT}/src/npp-qscintilla-static.pro"
        "-DNPP_QSCINTILLA_ROOT=${QSCINTILLA_ROOT}"
        "-DNPP_BOOSTREGEX_ROOT=${CMAKE_SOURCE_DIR}/third_party/boostregex"
        "-DNPP_LEXUSER_SOURCE=${CMAKE_SOURCE_DIR}/third_party/lexilla/LexUser.cxx"
        "-DNPP_QSCINTILLA_BUILD_DIR=${NPP_QSCINTILLA_BUILD_DIR}"
        "-DNPP_QSCINTILLA_CONFIG=${NPP_QSCINTILLA_CONFIG}"
        "-DNPP_QSCINTILLA_OUTPUT=${NPP_QSCINTILLA_ARCHIVE}"
        "-DNPP_QT_MAJOR_VERSION=5"
        "-DNPP_STATIC_LIBRARY_SUFFIX=${CMAKE_STATIC_LIBRARY_SUFFIX}"
        -P "${CMAKE_SOURCE_DIR}/cmake/BuildNppQScintilla.cmake"
    DEPENDS
        "${QSCINTILLA_ROOT}/src/npp-qscintilla-static.pro"
        "${QSCINTILLA_ROOT}/scintilla/include/SciLexer.h"
        "${QSCINTILLA_ROOT}/scintilla/include/Scintilla.h"
        "${QSCINTILLA_ROOT}/scintilla/include/Scintilla.iface"
        "${QSCINTILLA_ROOT}/scintilla/src/Catalogue.cpp"
        "${QSCINTILLA_ROOT}/scintilla/lexlib/StyleContext.h"
        "${QSCINTILLA_ROOT}/scintilla/lexlib/StyleContext.cpp"
        "${QSCINTILLA_ROOT}/scintilla/lexlib/WordList.h"
        "${CMAKE_SOURCE_DIR}/third_party/lexilla/LexUser.cxx"
        "${CMAKE_SOURCE_DIR}/third_party/boostregex/BoostRegExSearch.cxx"
        "${CMAKE_SOURCE_DIR}/third_party/boostregex/UTF8DocumentIterator.cxx"
        "${QSCINTILLA_ROOT}/src/qscintilla.pro"
        "${QSCINTILLA_ROOT}/src/qscidocument.cpp"
        "${QSCINTILLA_ROOT}/src/qsciscintillabase.cpp"
        "${QSCINTILLA_ROOT}/src/Qsci/qscidocument.h"
        "${QSCINTILLA_ROOT}/src/Qsci/qsciscintillabase.h"
    VERBATIM
)
add_custom_target(npp-qscintilla-build DEPENDS "${NPP_QSCINTILLA_ARCHIVE}")
add_library(npp-qscintilla STATIC IMPORTED GLOBAL)
set_target_properties(npp-qscintilla PROPERTIES
    IMPORTED_LOCATION "${NPP_QSCINTILLA_ARCHIVE}"
    INTERFACE_INCLUDE_DIRECTORIES "${QSCINTILLA_INCLUDE_DIR}"
)
add_dependencies(npp-qscintilla npp-qscintilla-build)

include_directories(
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/src/uchardet
    ${QSCINTILLA_INCLUDE_DIR}
)

# TinyXml 使用 STL wstring (generic_string = std::wstring)
add_compile_definitions(TIXML_USE_STL UNICODE _UNICODE)

# 创建输出目录
file(MAKE_DIRECTORY ${CMAKE_SOURCE_DIR}/src/MISC)
file(MAKE_DIRECTORY ${CMAKE_SOURCE_DIR}/src/WinControls/TabBar)
file(MAKE_DIRECTORY ${CMAKE_SOURCE_DIR}/src/WinControls/DockingWnd)
file(MAKE_DIRECTORY ${CMAKE_SOURCE_DIR}/src/Preferences)

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
    src/MISC/FileManager.cpp
    src/MISC/EncodingMapper.cpp
    src/MISC/TextFileCodec.cpp
    src/MISC/PlatformServices.cpp
    src/MISC/FileAssociationModel.cpp
    src/MISC/ConfigPathResolver.cpp
    src/MISC/ClosedFileHistory.cpp
    src/MISC/FunctionListParser.cpp
    src/MISC/ToolbarIconTheme.cpp
    src/Printing/NotepadPlusPrinter.cpp
    src/WinControls/TabBar/DocTabView.cpp
    src/WinControls/DockingWnd/FileBrowserPanel.cpp
    src/Preferences/PreferenceDlg.cpp
    src/WinControls/DockingWnd/DocumentMapPanel.cpp
    src/WinControls/DockingWnd/FunctionListPanel.cpp
    src/WinControls/ProjectPanel/WorkspaceDocument.cpp
    src/WinControls/ProjectPanel/ProjectPanel.cpp
    src/PluginSystem/PluginManager.cpp
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
    src/MISC/FileManager.h
    src/MISC/EncodingMapper.h
    src/MISC/TextFileCodec.h
    src/MISC/PlatformServices.h
    src/MISC/FileAssociationModel.h
    src/MISC/ConfigPathResolver.h
    src/MISC/ClosedFileHistory.h
    src/MISC/FunctionListParser.h
    src/MISC/ToolbarIconTheme.h
    src/Printing/NotepadPlusPrinter.h
    src/WinControls/TabBar/DocTabView.h
    src/WinControls/DockingWnd/FileBrowserPanel.h
    src/Preferences/PreferenceDlg.h
    src/WinControls/DockingWnd/DocumentMapPanel.h
    src/WinControls/DockingWnd/FunctionListPanel.h
    src/WinControls/ProjectPanel/WorkspaceDocument.h
    src/WinControls/ProjectPanel/ProjectPanel.h
    src/PluginSystem/IPlugin.h
    src/PluginSystem/PluginManager.h
)

# 资源文件
set(RESOURCES
    resources/resources.qrc
)

add_executable(notepadpp-qt WIN32
    ${SOURCES} ${HEADERS} ${RESOURCES} ${UCHARDET_SOURCES})

target_link_libraries(notepadpp-qt
    npp-qscintilla
    Qt5::Core
    Qt5::Gui
    Qt5::Widgets
    Qt5::Network
    Qt5::PrintSupport
    Qt5::Xml
)
if(WIN32)
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
    target_link_libraries(notepadpp-qt shell32 advapi32)
endif()

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
        NPP_PREFERENCES_SOURCE="${CMAKE_SOURCE_DIR}/src/Preferences/PreferenceDlg.cpp"
        NPP_FIND_LANGUAGE_SOURCE="${CMAKE_SOURCE_DIR}/resources/nativeLang/chineseSimplified.xml"
    )
    target_link_libraries(find-dialog-localization-tests Qt5::Core Qt5::Xml)
    add_test(NAME find-dialog-localization-tests
        COMMAND find-dialog-localization-tests)

    set(UI_PARITY_SOURCES ${SOURCES})
    list(REMOVE_ITEM UI_PARITY_SOURCES src/main.cpp)
    add_executable(ui-parity-capture
        tests/UiParityCapture.cpp
        ${UI_PARITY_SOURCES}
        ${HEADERS}
        ${RESOURCES}
        ${UCHARDET_SOURCES}
    )
    target_link_libraries(ui-parity-capture
        npp-qscintilla
        Qt5::Core
        Qt5::Gui
        Qt5::Widgets
        Qt5::Network
        Qt5::PrintSupport
        Qt5::Xml
    )
    if(WIN32)
        target_link_libraries(ui-parity-capture shell32 advapi32)
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

    add_executable(p1-runtime-capture
        tests/P1RuntimeCapture.cpp
        ${UI_PARITY_SOURCES}
        ${HEADERS}
        ${RESOURCES}
        ${UCHARDET_SOURCES}
    )
    target_link_libraries(p1-runtime-capture
        npp-qscintilla
        Qt5::Core
        Qt5::Gui
        Qt5::Widgets
        Qt5::Network
        Qt5::PrintSupport
        Qt5::Xml
    )
    if(WIN32)
        target_link_libraries(p1-runtime-capture shell32 advapi32)
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
        ${UCHARDET_SOURCES}
    )
    target_link_libraries(large-file-mode-tests
        npp-qscintilla
        Qt5::Core
        Qt5::Gui
        Qt5::Widgets
        Qt5::PrintSupport
    )
    target_include_directories(large-file-mode-tests PRIVATE
        "${QSCINTILLA_ROOT}/scintilla/include")
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
        npp-qscintilla
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
