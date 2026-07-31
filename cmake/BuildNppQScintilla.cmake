file(MAKE_DIRECTORY "${NPP_QSCINTILLA_BUILD_DIR}")

if(WIN32)
    set(path_separator ";")
else()
    set(path_separator ":")
endif()
set(ENV{PATH} "${NPP_COMPILER_BIN}${path_separator}$ENV{PATH}")

execute_process(
    COMMAND "${NPP_QMAKE_EXECUTABLE}" "${NPP_QSCINTILLA_PROJECT}"
        "CONFIG+=${NPP_QSCINTILLA_CONFIG}"
        "NPP_QSCINTILLA_ROOT=${NPP_QSCINTILLA_ROOT}"
        "NPP_BOOSTREGEX_SOURCE_ROOT=${NPP_BOOSTREGEX_ROOT}"
        "NPP_LEXILLA_ROOT=${NPP_LEXILLA_ROOT}"
    WORKING_DIRECTORY "${NPP_QSCINTILLA_BUILD_DIR}"
    RESULT_VARIABLE qmake_result
)
if(NOT qmake_result EQUAL 0)
    message(FATAL_ERROR "QScintilla qmake configuration failed (${qmake_result})")
endif()

execute_process(
    COMMAND "${NPP_QMAKE_BUILD_TOOL}"
    WORKING_DIRECTORY "${NPP_QSCINTILLA_BUILD_DIR}"
    RESULT_VARIABLE build_result
)
if(NOT build_result EQUAL 0)
    message(FATAL_ERROR "QScintilla static build failed (${build_result})")
endif()

file(GLOB_RECURSE qscintilla_archives
    "${NPP_QSCINTILLA_BUILD_DIR}/*qscintilla2_qt${NPP_QT_MAJOR_VERSION}_npp*${NPP_STATIC_LIBRARY_SUFFIX}"
)
list(REMOVE_ITEM qscintilla_archives "${NPP_QSCINTILLA_OUTPUT}")
list(LENGTH qscintilla_archives archive_count)
if(archive_count EQUAL 0)
    message(FATAL_ERROR "QScintilla static archive was not produced")
endif()

list(GET qscintilla_archives 0 qscintilla_archive)
get_filename_component(output_dir "${NPP_QSCINTILLA_OUTPUT}" DIRECTORY)
file(MAKE_DIRECTORY "${output_dir}")
execute_process(
    COMMAND "${CMAKE_COMMAND}" -E copy_if_different
            "${qscintilla_archive}" "${NPP_QSCINTILLA_OUTPUT}"
    RESULT_VARIABLE copy_result
)
if(NOT copy_result EQUAL 0)
    message(FATAL_ERROR "Could not stage QScintilla static archive (${copy_result})")
endif()

# Keep the declared custom-command output newer than all dependencies even
# when qmake determines that the archive contents did not change.
file(TOUCH "${NPP_QSCINTILLA_OUTPUT}")
