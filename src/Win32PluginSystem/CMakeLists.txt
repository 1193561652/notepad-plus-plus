# This directory is entered only from the top-level WIN32 branch.
# Keep the list explicit so new compatibility sources are reviewed deliberately.
if(NOT WIN32)
    return()
endif()

set(WIN32_PLUGIN_SYSTEM_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/Win32EditorMessageAdapter.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Win32MainWindowAdapter.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Win32MainEditorAdapter.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Win32SubEditorAdapter.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Win32PluginManager.cpp
)

set(WIN32_PLUGIN_SYSTEM_HEADERS
    ${CMAKE_CURRENT_SOURCE_DIR}/Win32EditorMessageAdapter.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Win32MainWindowAdapter.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Win32MainEditorAdapter.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Win32SubEditorAdapter.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Win32PluginInterface.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Win32PluginManager.h
)

set(SOURCES ${SOURCES} ${WIN32_PLUGIN_SYSTEM_SOURCES} PARENT_SCOPE)
set(HEADERS ${HEADERS} ${WIN32_PLUGIN_SYSTEM_HEADERS} PARENT_SCOPE)
