find_program(NPP_DOTNET_EXECUTABLE dotnet REQUIRED)
function(npp_managed_engine id repository assembly)
  set(engine_output "${PROJECT_BINARY_DIR}/plugins/${id}-qt/engine")
  file(GLOB_RECURSE engine_sources CONFIGURE_DEPENDS "${PROJECT_SOURCE_DIR}/${repository}/*.cs")
  list(FILTER engine_sources EXCLUDE REGEX "/(bin|obj)/")
  set(engine_project "${PROJECT_SOURCE_DIR}/${repository}/qt/engine/${assembly}.csproj")
  if(WIN32)
    find_package(Python3 REQUIRED COMPONENTS Interpreter)
    set(engine_binary "${engine_output}/${assembly}.exe")
    add_custom_command(OUTPUT "${engine_binary}"
      COMMAND "${Python3_EXECUTABLE}" -X utf8 "${PROJECT_SOURCE_DIR}/qt/common/build_framework_engine.py"
        --dotnet "${NPP_DOTNET_EXECUTABLE}" --project "${engine_project}" --output "${engine_binary}"
      DEPENDS ${engine_sources} "${engine_project}" qt/common/build_framework_engine.py qt/common/FrameworkJsonWire.cs VERBATIM)
    set(engine_destination "plugins/${id}-qt/engine")
  else()
    set(engine_binary "${engine_output}/${assembly}.dll")
    add_custom_command(OUTPUT "${engine_binary}"
      COMMAND ${CMAKE_COMMAND} -E env "DOTNET_CLI_HOME=${PROJECT_BINARY_DIR}/dotnet-home"
        DOTNET_CLI_TELEMETRY_OPTOUT=1 DOTNET_ADD_GLOBAL_TOOLS_TO_PATH=0
        "${NPP_DOTNET_EXECUTABLE}" publish "${engine_project}"
        --configuration Release --output "${engine_output}" --nologo
        "-p:BaseIntermediateOutputPath=${PROJECT_BINARY_DIR}/${id}-engine-obj/"
      DEPENDS ${engine_sources} "${engine_project}" VERBATIM)
    if(APPLE)
      set(engine_destination "plugins/${id}-qt/macos/engine")
    else()
      set(engine_destination "plugins/${id}-qt/linux/engine")
    endif()
  endif()
  add_custom_target(${id}Engine DEPENDS "${engine_binary}")
  add_dependencies(${id} ${id}Engine)
  if(WIN32)
    install(DIRECTORY "${engine_output}/" DESTINATION "${engine_destination}" FILES_MATCHING PATTERN "*.exe" PATTERN "*.config")
  else()
    install(DIRECTORY "${engine_output}/" DESTINATION "${engine_destination}" FILES_MATCHING PATTERN "*.dll" PATTERN "*.json")
  endif()
endfunction()
