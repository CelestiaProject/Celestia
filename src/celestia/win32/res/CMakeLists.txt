if(NOT ENABLE_NLS)
  return()
endif()

find_package(Perl)

if(PERL_FOUND)
  include(windres)

  file(GLOB PO_FILES2 "${CMAKE_SOURCE_DIR}/po/*.po")
  # Georgian does not have an associated 8-bit codepage
  # If the Windows frontend is rewritten to use wchar APIs this can be re-enabled
  list(REMOVE_ITEM PO_FILES2 "${CMAKE_SOURCE_DIR}/po/ka.po")
  WINDRES_CREATE_TRANSLATIONS(celestia.rc ALL ${PO_FILES2})
else()
  message("Perl not found, skipping generation of translations")
endif()
