add_executable(plugin-abi-tests PluginAbiTests.cpp)
target_include_directories(plugin-abi-tests PRIVATE
  "${NPP_QT_HOST_SOURCE}/src/CrossPlatformPluginSystem"
  "${NPP_QT_HOST_SOURCE}/third_party/scintilla/include"
  "${PROJECT_SOURCE_DIR}/selectquotedtext/NPP")
target_link_libraries(plugin-abi-tests PRIVATE Qt${QT_VERSION_MAJOR}::Widgets)
set(plugin_ids qkNppReverseLines SelectQuotedText mimeTools nppAutoDetectIndent SelectToClipboard BracketsCheck Remove_dup_lines SurroundSelection SecurePad ElasticTabstops Merge_files_in_one urlPlugin BetterMultiSelection nppConverter EditorConfig JSONViewer GotoLineCol)
set(plugin_tests original-core-regression)
add_executable(original-core-regression CoreRegressionTests.cpp
  ../../SecurePad/blowfish.cpp ../../mimetools/src/b64.cpp ../../mimetools/src/qp.cpp
  ../../mimetools/src/url.cpp ../../mimetools/src/saml.cpp ../../mimetools/src/tinflate.c)
target_link_libraries(original-core-regression PRIVATE Qt${QT_VERSION_MAJOR}::Core)
target_compile_definitions(original-core-regression PRIVATE NPP_QT_PORT)
add_test(NAME original-core-regression COMMAND original-core-regression)
foreach(id ${plugin_ids})
  add_dependencies(plugin-abi-tests ${id})
  add_test(NAME ${id}-abi COMMAND plugin-abi-tests $<TARGET_FILE:${id}> ${id})
  list(APPEND plugin_tests ${id}-abi)
endforeach()
get_target_property(qmake Qt${QT_VERSION_MAJOR}::qmake IMPORTED_LOCATION)
set(NPP_QT_HOST_BUILD "" CACHE PATH "Optional matching host build for real Scintilla tests")
if(NPP_QT_HOST_BUILD)
  find_library(npp_scintilla_library NAMES npp-scintilla-qt PATHS "${NPP_QT_HOST_BUILD}" NO_DEFAULT_PATH REQUIRED)
  add_executable(plugin-scintilla-tests ScintillaIntegrationTests.cpp)
  target_compile_definitions(plugin-scintilla-tests PRIVATE SCINTILLA_QT EXPORT_IMPORT_API=)
  target_include_directories(plugin-scintilla-tests PRIVATE
    "${NPP_QT_HOST_SOURCE}/src/CrossPlatformPluginSystem"
    "${NPP_QT_HOST_SOURCE}/third_party/scintilla/include"
    "${NPP_QT_HOST_SOURCE}/third_party/scintilla/src"
    "${NPP_QT_HOST_SOURCE}/third_party/scintilla/qt/ScintillaEditBase")
  target_link_libraries(plugin-scintilla-tests PRIVATE "${npp_scintilla_library}" Qt${QT_VERSION_MAJOR}::Widgets)
  foreach(id ${plugin_ids})
    add_dependencies(plugin-scintilla-tests ${id})
    add_test(NAME ${id}-scintilla COMMAND plugin-scintilla-tests $<TARGET_FILE:${id}> ${id})
    list(APPEND plugin_tests ${id}-scintilla)
  endforeach()
  add_test(NAME GotoLineCol-command-line COMMAND plugin-scintilla-tests $<TARGET_FILE:GotoLineCol> GotoLineCol -n2 -c3 test.cpp)
  list(APPEND plugin_tests GotoLineCol-command-line)
endif()
get_filename_component(qt_bin "${qmake}" DIRECTORY)
if(WIN32)
  string(REPLACE ";" "\\;" test_path "$ENV{PATH}")
  set_tests_properties(${plugin_tests} PROPERTIES
    # Qt 5.12's offscreen/minimal Windows backends can stall in QMessageBox::exec.
    # Exercise the deployed Windows backend, including modal dialog dismissal.
    ENVIRONMENT "QT_QPA_PLATFORM=windows;QT_FORCE_STDERR_LOGGING=1;PATH=${qt_bin}\\;${test_path}")
else()
  set_tests_properties(${plugin_tests} PROPERTIES
    ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
endif()
