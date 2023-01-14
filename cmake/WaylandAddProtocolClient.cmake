function(wayland_add_protocol_client _sources _protocol)
    if (NOT TARGET Wayland::Scanner)
        message(FATAL "The wayland-scanner executable has not been found on your system.")
    endif()

    if (NOT Wayland_PROTOCOLS_DATADIR)
        message(FATAL "The wayland-protocols data directory has not been found on your system.")
    endif()

    get_filename_component(_basename "${_protocol}" NAME_WLE)

    set(_source_file "${Wayland_PROTOCOLS_DATADIR}/${_protocol}")
    set(_client_header "${CMAKE_CURRENT_BINARY_DIR}/${_basename}-client-protocol.h")
    set(_private_code "${CMAKE_CURRENT_BINARY_DIR}/${_basename}-protocol.c")

    add_custom_command(OUTPUT "${_client_header}"
        COMMAND Wayland::Scanner client-header < ${_source_file} > ${_client_header}
        DEPENDS ${_source_file} VERBATIM)

    add_custom_command(OUTPUT "${_private_code}"
        COMMAND Wayland::Scanner private-code < ${_source_file} > ${_private_code}
        DEPENDS ${_source_file} VERBATIM)

    list(APPEND ${_sources} "${_private_code}" "${_client_header}")
    set(${_sources} ${${_sources}} PARENT_SCOPE)
endfunction()
