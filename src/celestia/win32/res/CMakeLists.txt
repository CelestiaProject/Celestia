if((NOT ENABLE_NLS) OR (NOT GETTEXT_FOUND))
  return()
endif()

find_package(Python3 COMPONENTS Interpreter)
if(NOT Python3_Interpreter_FOUND)
  message("Python 3 not found, skipping generation of translated resources")
endif()

add_custom_target(wintranslations ALL)

file(GLOB PO_FILES "${CMAKE_SOURCE_DIR}/po/*.po")

set(CELESTIA_RC "${CMAKE_CURRENT_SOURCE_DIR}/celestia.rc")
set(UPDATERC_SCRIPT "${CMAKE_SOURCE_DIR}/po/updaterc.py")

foreach(PO_FILE IN LISTS PO_FILES)
  get_filename_component(PO_LANGUAGE "${PO_FILE}" NAME_WLE)
  if(NOT TARGET translation_${PO_LANGUAGE})
    message(FATAL_ERROR "No translation target defined for ${PO_LANGUAGE}")
  endif()
  set(GMO_FILE "${CMAKE_BINARY_DIR}/po/${PO_LANGUAGE}.gmo")
  set(RC_FILE "${CMAKE_CURRENT_BINARY_DIR}/celestia_${PO_LANGUAGE}.rc")
  add_custom_command(
    OUTPUT "${RC_FILE}"
    COMMAND Python3::Interpreter
            ${UPDATERC_SCRIPT}
            ${CELESTIA_RC}
            ${GMO_FILE}
            -o ${RC_FILE}
    DEPENDS "${PO_FILE}" "${CELESTIA_RC}" "${UPDATERC_SCRIPT}" translation_${PO_LANGUAGE}
    VERBATIM
  )

  add_library(res_${PO_LANGUAGE} MODULE ${RC_FILE})
  set_target_properties(res_${PO_LANGUAGE} PROPERTIES PREFIX "" LINKER_LANGUAGE C)
  if(MSVC)
    set_target_properties(res_${PO_LANGUAGE} PROPERTIES LINK_FLAGS "/MANIFEST:NO /NODEFAULTLIB /NOENTRY")
  endif()

  add_dependencies(wintranslations res_${PO_LANGUAGE})

  install(
    TARGETS res_${PO_LANGUAGE}
    LIBRARY DESTINATION ${CMAKE_INSTALL_BINDIR}/locale
    COMPONENT core
  )
endforeach()
