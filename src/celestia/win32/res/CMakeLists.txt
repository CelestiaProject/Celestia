if(NOT ENABLE_NLS)
  return()
endif()

find_package(Perl)

if(PERL_FOUND)
  include(windres)

  file(GLOB PO_FILES2 "${CMAKE_SOURCE_DIR}/po/*.po")
  WINDRES_CREATE_TRANSLATIONS(celestia.rc ALL ${PO_FILES2})
else()
  message("Perl not found, skipping generation of translations")
endif()
