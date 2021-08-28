include(windres)

file(GLOB PO_FILES2 "${CMAKE_SOURCE_DIR}/po/*.po")
WINDRES_CREATE_TRANSLATIONS(celestia.rc ALL ${PO_FILES2})
