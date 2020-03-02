macro(GETTEXT_CREATE_TRANSLATIONS2 _potFile _firstPoFileArg)
   # make it a real variable, so we can modify it here
   set(_firstPoFile "${_firstPoFileArg}")

   set(_gmoFiles)
   get_filename_component(_potName ${_potFile} NAME)
   string(REGEX REPLACE "^(.+)(\\.[^.]+)$" "\\1" _potBasename ${_potName})
   get_filename_component(_absPotFile ${_potFile} ABSOLUTE)

   set(_addToAll)
   if(${_firstPoFile} STREQUAL "ALL")
      set(_addToAll "ALL")
      set(_firstPoFile)
   endif()

   foreach (_currentPoFile ${_firstPoFile} ${ARGN})
      get_filename_component(_absFile ${_currentPoFile} ABSOLUTE)
      get_filename_component(_abs_PATH ${_absFile} PATH)
      get_filename_component(_lang ${_absFile} NAME_WE)
      set(_gmoFile ${CMAKE_CURRENT_BINARY_DIR}/${_lang}.gmo)
      set(_poFile ${CMAKE_CURRENT_BINARY_DIR}/${_lang}.po)

      add_custom_command(
         OUTPUT ${_gmoFile}
         COMMAND ${GETTEXT_MSGMERGE_EXECUTABLE} --quiet --output-file=${_poFile} --lang=${_lang} --sort-output ${_absFile} ${_absPotFile}
         COMMAND ${GETTEXT_MSGFMT_EXECUTABLE} -o ${_gmoFile} ${_poFile}
         DEPENDS ${_absPotFile} ${_absFile}
      )

      if(NATIVE_OSX_APP)
        install(FILES ${_gmoFile} DESTINATION Resources/locale/${_lang}/LC_MESSAGES RENAME ${_potBasename}.mo)
      else()
        install(FILES ${_gmoFile} DESTINATION ${CMAKE_INSTALL_LOCALEDIR}/${_lang}/LC_MESSAGES RENAME ${_potBasename}.mo)
      endif()
      set(_gmoFiles ${_gmoFiles} ${_gmoFile})

   endforeach ()

   if(NOT TARGET translations)
      add_custom_target(translations)
   endif()

  _GETTEXT_GET_UNIQUE_TARGET_NAME(translations uniqueTargetName)

   add_custom_target(${uniqueTargetName} ${_addToAll} DEPENDS ${_gmoFiles})

   add_dependencies(translations ${uniqueTargetName})

endmacro()
