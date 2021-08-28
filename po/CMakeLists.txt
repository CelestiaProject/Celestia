if((NOT ENABLE_NLS) OR (NOT GETTEXT_FOUND))
  return()
endif()

file(GLOB PO_FILES "*.po")
gettext_create_translations2("celestia.pot" ALL ${PO_FILES})

if(ENABLE_WIN)
  set(OPTIONS "")
  if("${CMAKE_GENERATOR_PLATFORM}" STREQUAL "x64")
    set(OPTIONS "${OPTIONS} --platform=x64")
  endif()
  if(MINGW)
    set(OPTIONS "${OPTIONS} --mingw")
  endif()
  execute_process(COMMAND "${CMAKE_CURRENT_SOURCE_DIR}/translate_resources.pl" "${OPTIONS}"
                  WORKING_DIRECTORY CMAKE_BINARY_DIR)
endif()
