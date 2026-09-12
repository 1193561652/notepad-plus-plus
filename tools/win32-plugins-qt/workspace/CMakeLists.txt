cmake_minimum_required(VERSION 3.16)
project(NppWin32PluginsQt LANGUAGES C CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Core Widgets)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core Widgets)
include(CTest)
set(NPP_QT_HOST_SOURCE "${CMAKE_CURRENT_SOURCE_DIR}/../notepad-plus-plus" CACHE PATH "Notepad++ Qt source (public plugin and Scintilla headers)")
if(NOT EXISTS "${NPP_QT_HOST_SOURCE}/src/CrossPlatformPluginSystem/PluginInterface.h")
  message(FATAL_ERROR "Set NPP_QT_HOST_SOURCE to the Notepad++ Qt source directory")
endif()
function(npp_qt_plugin id source)
  add_library(${id} MODULE ${source} ${ARGN})
  target_compile_definitions(${id} PRIVATE NPP_PLUGIN_BUILD)
  target_include_directories(${id} PRIVATE
    "${PROJECT_SOURCE_DIR}/qt/common"
    "${NPP_QT_HOST_SOURCE}/src/CrossPlatformPluginSystem"
    "${NPP_QT_HOST_SOURCE}/third_party/scintilla/include")
  target_link_libraries(${id} PRIVATE Qt${QT_VERSION_MAJOR}::Core Qt${QT_VERSION_MAJOR}::Widgets)
  set_target_properties(${id} PROPERTIES PREFIX "" OUTPUT_NAME "${id}-qt"
    LIBRARY_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/plugins/${id}-qt"
    RUNTIME_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/plugins/${id}-qt")
  if(WIN32)
    set(platform_path "")
  elseif(APPLE)
    set(platform_path "/macos")
    set_target_properties(${id} PROPERTIES SUFFIX ".dylib")
  else()
    set(platform_path "/linux")
  endif()
  install(TARGETS ${id} LIBRARY DESTINATION "plugins/${id}-qt${platform_path}"
    RUNTIME DESTINATION "plugins/${id}-qt${platform_path}")
endfunction()
npp_qt_plugin(qkNppReverseLines qkNppReverseLines/qt/Plugin.cpp)
npp_qt_plugin(PluginDemo plugindemo/qt/Plugin.cpp plugindemo/qt/Resources.qrc)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Xml)
find_program(NPP_DOTNET_EXECUTABLE dotnet REQUIRED)
npp_qt_plugin(RandomValues RandomValuesNPP/qt/Plugin.cpp RandomValuesNPP/qt/Resources.qrc)
set_target_properties(RandomValues PROPERTIES AUTORCC ON)
include(qt/common/ManagedEngine.cmake)
npp_managed_engine(RandomValues RandomValuesNPP RandomValues.Engine)
npp_qt_plugin(JsonTools JsonToolsNppPlugin/qt/Plugin.cpp JsonToolsNppPlugin/qt/Resources.qrc)
set_target_properties(JsonTools PROPERTIES AUTORCC ON)
npp_managed_engine(JsonTools JsonToolsNppPlugin JsonTools.Engine)
install(FILES JsonToolsNppPlugin/LICENSE.md DESTINATION plugins/JsonTools-qt/licenses)
file(COPY JsonToolsNppPlugin/testfiles DESTINATION "${PROJECT_BINARY_DIR}/plugins/JsonTools-qt")
install(DIRECTORY JsonToolsNppPlugin/testfiles DESTINATION plugins/JsonTools-qt)
install(FILES RandomValuesNPP/readme.md DESTINATION plugins/RandomValues-qt/licenses)
file(GLOB xml_formatter_sources CONFIGURE_DEPENDS xmltools/QuickXmlLib/QuickXml/src/*.cpp xmltools/SimpleXmlLib/SimpleXml/src/*.cpp xmltools/StringXmlLib/StringXml/src/*.cpp)
npp_qt_plugin(XMLTools xmltools/qt/Plugin.cpp xmltools/qt/Resources.qrc xmltools/ToolsPrettyPrintFast.cpp xmltools/ToolsConvert.cpp xmltools/ToolsComment.cpp xmltools/Config.cpp ${xml_formatter_sources})
target_include_directories(XMLTools PRIVATE xmltools/QuickXmlLib/QuickXml/src xmltools/SimpleXmlLib/SimpleXml/src xmltools/StringXmlLib/StringXml/src)
target_compile_definitions(XMLTools PRIVATE NPP_QT_PORT)
set_target_properties(XMLTools PROPERTIES AUTOMOC ON AUTORCC ON)
install(FILES xmltools/LICENSE DESTINATION plugins/XMLTools-qt/licenses)
if(WIN32)
  add_custom_command(OUTPUT "${PROJECT_BINARY_DIR}/plugins/XMLTools-qt/engine/XmlToolsBackend.exe"
    COMMAND ${CMAKE_COMMAND} -S "${PROJECT_SOURCE_DIR}/xmltools/qt/engine" -B "${PROJECT_BINARY_DIR}/xml-backend"
      -G "Visual Studio 17 2022" -A x64
      "-DRAPIDJSON_INCLUDE=${PROJECT_SOURCE_DIR}/JSON-Viewer/external/rapidjson/include"
      "-DBACKEND_OUTPUT=${PROJECT_BINARY_DIR}/plugins/XMLTools-qt/engine"
    COMMAND ${CMAKE_COMMAND} --build "${PROJECT_BINARY_DIR}/xml-backend" --config Release
    DEPENDS xmltools/MSXMLWrapper.cpp xmltools/MSXMLHelper.cpp xmltools/XmlWrapperInterface.h xmltools/Config.h
      xmltools/qt/engine/Host.h xmltools/qt/engine/Main.cpp xmltools/qt/engine/CMakeLists.txt VERBATIM)
  add_custom_target(XMLToolsBackend DEPENDS "${PROJECT_BINARY_DIR}/plugins/XMLTools-qt/engine/XmlToolsBackend.exe")
  add_dependencies(XMLTools XMLToolsBackend)
  install(FILES "${PROJECT_BINARY_DIR}/plugins/XMLTools-qt/engine/XmlToolsBackend.exe" DESTINATION plugins/XMLTools-qt/engine)
endif()
npp_qt_plugin(SessionMgr npp-session-manager/qt/Plugin.cpp)
target_link_libraries(SessionMgr PRIVATE Qt${QT_VERSION_MAJOR}::Xml)
set_target_properties(SessionMgr PROPERTIES AUTOMOC ON)
install(FILES npp-session-manager/license.txt DESTINATION plugins/SessionMgr-qt/licenses)
npp_qt_plugin(DoxyIt DoxyIt/qt/Plugin.cpp DoxyIt/qt/UserInfo.cpp DoxyIt/src/Parsers/Parsers.cpp
  DoxyIt/src/Parsers/C_Parser.cpp DoxyIt/src/Parsers/Python_Parser.cpp DoxyIt/src/Parsers/Null_Parser.cpp
  DoxyIt/src/Utilities/Utils.cpp DoxyIt/src/Utilities/trex.cpp DoxyIt/src/JumpLocations.cpp)
target_compile_definitions(DoxyIt PRIVATE NPP_QT_PORT)
target_include_directories(DoxyIt PRIVATE DoxyIt/src DoxyIt/src/Parsers DoxyIt/src/Utilities)
set_target_properties(DoxyIt PROPERTIES AUTOMOC ON)
if(WIN32)
  target_link_libraries(DoxyIt PRIVATE secur32)
endif()
install(FILES DoxyIt/LICENSE DESTINATION plugins/DoxyIt-qt/licenses)
npp_qt_plugin(CodeAlignment codealignment/qt/Plugin.cpp codealignment/qt/Resources.qrc)
target_link_libraries(CodeAlignment PRIVATE Qt${QT_VERSION_MAJOR}::Xml)
set_target_properties(CodeAlignment PROPERTIES AUTORCC ON)
install(FILES codealignment/CodeAlignment.VisualStudio/license.txt DESTINATION plugins/CodeAlignment-qt/licenses)
set_target_properties(PluginDemo PROPERTIES AUTOMOC ON AUTORCC ON)
install(FILES plugindemo/license.txt DESTINATION plugins/PluginDemo-qt/licenses)
npp_qt_plugin(SelectQuotedText selectquotedtext/qt/Plugin.cpp)
set(MIME_CORE mimetools/src/b64.cpp mimetools/src/qp.cpp mimetools/src/url.cpp
  mimetools/src/saml.cpp mimetools/src/tinflate.c)
npp_qt_plugin(mimeTools mimetools/qt/Plugin.cpp ${MIME_CORE})
npp_qt_plugin(nppAutoDetectIndent nppAutoDetectIndent/qt/Plugin.cpp nppAutoDetectIndent/src/settings.cpp)
npp_qt_plugin(SelectToClipboard SelectToClipboard/qt/Plugin.cpp)
npp_qt_plugin(BracketsCheck BracketsCheck/qt/Plugin.cpp)
npp_qt_plugin(Remove_dup_lines Remove_dup_lines/qt/Plugin.cpp)
npp_qt_plugin(SurroundSelection SurroundSelection/qt/Plugin.cpp)
npp_qt_plugin(SecurePad SecurePad/qt/Plugin.cpp SecurePad/blowfish.cpp)
npp_qt_plugin(ElasticTabstops ElasticTabstops/qt/Plugin.cpp ElasticTabstops/src/ElasticTabstops.cpp)
target_compile_definitions(ElasticTabstops PRIVATE NPP_QT_PORT)
npp_qt_plugin(Merge_files_in_one Merge-files-in-one/qt/Plugin.cpp)
npp_qt_plugin(urlPlugin nppURLPlugin/qt/Plugin.cpp nppURLPlugin/qt/Resources.qrc)
set_target_properties(urlPlugin PROPERTIES AUTORCC ON)
npp_qt_plugin(BetterMultiSelection BetterMultiSelection/qt/Plugin.cpp)
npp_qt_plugin(nppConverter converter/qt/Plugin.cpp)
add_subdirectory(editorconfig-notepad-plus-plus/qt)
npp_qt_plugin(JSONViewer JSON-Viewer/qt/Plugin.cpp JSON-Viewer/src/NppJsonViewer/JsonHandler.cpp
  JSON-Viewer/src/NppJsonViewer/RapidJsonHandler.cpp JSON-Viewer/qt/Resources.qrc)
target_compile_definitions(JSONViewer PRIVATE NPP_QT_PORT)
target_include_directories(JSONViewer PRIVATE "${PROJECT_SOURCE_DIR}/JSON-Viewer/external/rapidjson/include")
set_target_properties(JSONViewer PROPERTIES AUTORCC ON)
npp_qt_plugin(GotoLineCol Goto-Line-Col-NPP-Plugin/qt/Plugin.cpp Goto-Line-Col-NPP-Plugin/qt/Resources.qrc)
set_target_properties(GotoLineCol PROPERTIES AUTORCC ON)
if(NOT EXISTS "${PROJECT_SOURCE_DIR}/JSON-Viewer/external/rapidjson/include/rapidjson/reader.h")
  message(FATAL_ERROR "Initialize JSON-Viewer/external/rapidjson at the repository gitlink; see qt/README.md")
endif()
install(FILES JSON-Viewer/LICENSE DESTINATION plugins/JSONViewer-qt/licenses RENAME JSON-Viewer.txt)
install(FILES JSON-Viewer/external/rapidjson/license.txt DESTINATION plugins/JSONViewer-qt/licenses RENAME RapidJSON.txt)
if(QT_VERSION_MAJOR EQUAL 6)
  find_package(Qt6 REQUIRED COMPONENTS Core5Compat)
  target_link_libraries(BetterMultiSelection PRIVATE Qt6::Core5Compat)
endif()

if(BUILD_TESTING)
  add_subdirectory(qt/tests)
endif()

foreach(entry
  "qkNppReverseLines|qkNppReverseLines/LICENSE"
  "SelectQuotedText|selectquotedtext/LICENSE.md"
  "mimeTools|mimetools/license.txt"
  "nppAutoDetectIndent|nppAutoDetectIndent/LICENSE"
  "SelectToClipboard|SelectToClipboard/license.txt"
  "BracketsCheck|BracketsCheck/LICENSE"
  "SurroundSelection|SurroundSelection/LICENSE"
  "SecurePad|SecurePad/LICENSE"
  "ElasticTabstops|ElasticTabstops/LICENSE"
  "urlPlugin|nppURLPlugin/LICENSE"
  "BetterMultiSelection|BetterMultiSelection/LICENSE"
  "nppConverter|converter/license.txt"
  "GotoLineCol|Goto-Line-Col-NPP-Plugin/LICENSE"
  "EditorConfig|editorconfig-notepad-plus-plus/COPYING.txt")
  string(REPLACE "|" ";" parts "${entry}")
  list(GET parts 0 id)
  list(GET parts 1 license)
  install(FILES "${license}" DESTINATION "plugins/${id}-qt/licenses")
endforeach()
install(FILES qt/README.md qt/PORTING_STATUS.md DESTINATION plugins/qt-port-docs)
