if((NOT ENABLE_NLS) OR (NOT GETTEXT_FOUND))
  return()
endif()

file(GLOB PO_FILES "*.po")
gettext_create_translations2("celestia_constellations.pot" ALL ${PO_FILES})
