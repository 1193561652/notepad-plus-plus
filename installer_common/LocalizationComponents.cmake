# Common localization payload and component metadata.
#
# npp_install_localizations(<destination> COMPONENTIZED)
#   Installs English with the required Application component and exposes every
#   other official language as an initially unselected Localization component.
#
# npp_install_localizations(<destination>)
#   Installs every official language with the required Application component.

macro(npp_install_localizations destination)
    set(_npp_localization_componentized FALSE)
    if("${ARGN}" STREQUAL "COMPONENTIZED")
        set(_npp_localization_componentized TRUE)
    endif()

    file(GLOB _npp_language_files CONFIGURE_DEPENDS
        "${CMAKE_SOURCE_DIR}/installer_common/nativeLang/*.xml")
    list(SORT _npp_language_files)

    foreach(_npp_language_file IN LISTS _npp_language_files)
        get_filename_component(_npp_language_id
            "${_npp_language_file}" NAME_WE)

        # The official XML name is the same label shown by Notepad++ itself.
        file(READ "${_npp_language_file}" _npp_language_xml LIMIT 16384)
        unset(CMAKE_MATCH_1)
        string(REGEX MATCH
            "Native-Langue[^>]*name[ \t]*=[ \t]*\"([^\"]+)\""
            _npp_language_name_match "${_npp_language_xml}")
        if(CMAKE_MATCH_1)
            set(_npp_language_display_name "${CMAKE_MATCH_1}")
        else()
            set(_npp_language_display_name "${_npp_language_id}")
        endif()

        if(_npp_localization_componentized
           AND NOT _npp_language_id STREQUAL "english")
            string(MAKE_C_IDENTIFIER "Language_${_npp_language_id}"
                _npp_language_component)
            string(TOUPPER "${_npp_language_component}"
                _npp_language_component_upper)
            install(FILES "${_npp_language_file}"
                DESTINATION "${destination}"
                COMPONENT "${_npp_language_component}")
            set("CPACK_COMPONENT_${_npp_language_component_upper}_DISPLAY_NAME"
                "${_npp_language_display_name}")
            set("CPACK_COMPONENT_${_npp_language_component_upper}_DESCRIPTION"
                "Install the ${_npp_language_display_name} user-interface language file.")
            set("CPACK_COMPONENT_${_npp_language_component_upper}_GROUP"
                "Localization")
            set("CPACK_COMPONENT_${_npp_language_component_upper}_DISABLED" TRUE)
            list(APPEND NPP_OPTIONAL_LANGUAGE_COMPONENTS
                "${_npp_language_component}")
        else()
            install(FILES "${_npp_language_file}"
                DESTINATION "${destination}"
                COMPONENT Application)
        endif()
    endforeach()

    unset(_npp_localization_componentized)
    unset(_npp_language_files)
    unset(_npp_language_file)
    unset(_npp_language_id)
    unset(_npp_language_xml)
    unset(_npp_language_name_match)
    unset(_npp_language_display_name)
    unset(_npp_language_component)
    unset(_npp_language_component_upper)
endmacro()
