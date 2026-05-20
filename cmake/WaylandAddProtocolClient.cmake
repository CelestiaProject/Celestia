function(wayland_find_protocol_definition)
  cmake_parse_arguments(PARSE_ARGV 1 arg "" "NAME" "PATHS")
  foreach(rel_path IN LISTS arg_PATHS)
    list(APPEND abs_paths "${Wayland_PROTOCOLS_DATADIR}/${rel_path}")
  endforeach()

  find_file(
    _PROTOCOL_FILE
    ${arg_NAME}
    PATHS ${abs_paths}
  )
  if(_PROTOCOL_FILE STREQUAL "PROTOCOL_FILE-NOTFOUND")
    message(STATUS "Wayland protocol file ${arg_NAME} not found")
  else()
    message(STATUS "Wayland protocol file ${arg_NAME} found")
    set(${ARGV0} "${_PROTOCOL_FILE}" PARENT_SCOPE)
  endif()

  unset(_PROTOCOL_FILE CACHE)
endfunction()

function(wayland_add_protocol_client _sources _protocol)
  if (NOT TARGET Wayland::Scanner)
    message(FATAL_ERROR "The wayland-scanner executable has not been found on your system.")
  endif()

  if (NOT Wayland_PROTOCOLS_DATADIR)
    message(FATAL_ERROR "The wayland-protocols data directory has not been found on your system.")
  endif()

  get_filename_component(_basename "${_protocol}" NAME_WLE)

  set(_client_header "${CMAKE_CURRENT_BINARY_DIR}/${_basename}-client-protocol.h")
  set(_private_code "${CMAKE_CURRENT_BINARY_DIR}/${_basename}-protocol.c")

  add_custom_command(OUTPUT "${_client_header}"
    COMMAND Wayland::Scanner client-header < ${_protocol} > ${_client_header}
    DEPENDS ${_protocol} VERBATIM
  )

  add_custom_command(OUTPUT "${_private_code}"
    COMMAND Wayland::Scanner private-code < ${_protocol} > ${_private_code}
    DEPENDS ${_protocol} VERBATIM
  )

  list(APPEND ${_sources} "${_private_code}" "${_client_header}")
  set(${_sources} ${${_sources}} PARENT_SCOPE)
endfunction()
