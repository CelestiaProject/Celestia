if((NOT ENABLE_NLS) OR (NOT GETTEXT_FOUND))
  return()
endif()

add_custom_target(translations ALL)

file(GLOB PO_FILES "*.po")

foreach(PO_FILE IN LISTS PO_FILES)
  get_filename_component(PO_LANGUAGE "${PO_FILE}" NAME_WLE)
  set(GMO_FILE "${CMAKE_CURRENT_BINARY_DIR}/${PO_LANGUAGE}.gmo")
  add_custom_command(
    OUTPUT "${GMO_FILE}"
    COMMAND ${GETTEXT_MSGFMT_EXECUTABLE} -o ${GMO_FILE} ${PO_FILE}
    DEPENDS "${PO_FILE}"
    VERBATIM
  )

  add_custom_target(translation_${PO_LANGUAGE} DEPENDS "${GMO_FILE}")
  add_dependencies(translations translation_${PO_LANGUAGE})

  install(
    FILES "${GMO_FILE}"
    DESTINATION ${CMAKE_INSTALL_LOCALEDIR}/${PO_LANGUAGE}/LC_MESSAGES
    RENAME celestia.mo
    COMPONENT core
  )
endforeach()
