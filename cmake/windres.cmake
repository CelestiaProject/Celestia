function(_WINDRES_GET_UNIQUE_TARGET_NAME _name _unique_name)
  set(propertyName "_WINDRES_UNIQUE_COUNTER_${_name}")
  get_property(currentCounter GLOBAL PROPERTY "${propertyName}")
  if(NOT currentCounter)
    set(currentCounter 1)
  endif()
  set(${_unique_name} "${_name}_${currentCounter}" PARENT_SCOPE)
  math(EXPR currentCounter "${currentCounter} + 1")
  set_property(GLOBAL PROPERTY ${propertyName} ${currentCounter} )
endfunction()

macro(WINDRES_CREATE_TRANSLATIONS _rcFile _firstPoFileArg)
  # make it a real variable, so we can modify it here
  set(_firstPoFile "${_firstPoFileArg}")

  set(_addToAll)
  if(${_firstPoFile} STREQUAL "ALL")
    set(_addToAll "ALL")
    set(_firstPoFile)
  endif()

  set(_dllFiles)
  foreach(_currentPoFile ${_firstPoFile} ${ARGN})
    get_filename_component(_absFile ${_currentPoFile} ABSOLUTE)
    get_filename_component(_lang ${_absFile} NAME_WE)
    set(_locRcFile ${CMAKE_CURRENT_BINARY_DIR}/celestia_${_lang}.rc)
    set(_dllFile res_${_lang})

    add_custom_command(
      OUTPUT ${_locRcFile}
      COMMAND perl ${CMAKE_SOURCE_DIR}/po/translate_resources.pl ${_lang}
      DEPENDS ${_absFile}
    )

    add_library(${_dllFile} MODULE ${_locRcFile})
    set_target_properties(${_dllFile} PROPERTIES PREFIX "" LINKER_LANGUAGE C)
    if(MSVC)
      set_target_properties(${_dllFile} PROPERTIES LINK_FLAGS "/MANIFEST:NO /NODEFAULTLIB /NOENTRY")
    endif()

    install(TARGETS ${_dllFile} LIBRARY DESTINATION ${CMAKE_INSTALL_BINDIR})
    set(_dllFiles ${_dllFiles} ${_dllFile})
  endforeach()

  if(NOT TARGET resources)
    add_custom_target(resources)
  endif()

  _WINDRES_GET_UNIQUE_TARGET_NAME(resources uniqueTargetName)
  add_custom_target(${uniqueTargetName} ${_addToAll} DEPENDS ${_dllFiles})
  add_dependencies(resources ${uniqueTargetName})
endmacro()
