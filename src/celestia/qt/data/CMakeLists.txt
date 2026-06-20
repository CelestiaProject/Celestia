set(METAINFO_ID "space.celestiaproject.celestia_qt6")
set(METAINFO_NAME "Celestia (Qt6)")
set(METAINFO_DESKTOP "space.celestiaproject.celestia_qt6.desktop")
configure_file(
  "../../data/space.celestiaproject.Celestia.metainfo.xml.in"
  "${CMAKE_CURRENT_BINARY_DIR}/${METAINFO_ID}.metainfo.xml"
)

install(
  FILES "${METAINFO_DESKTOP}"
  DESTINATION "${CMAKE_INSTALL_DATADIR}/applications"
  COMPONENT qt6gui
)
install(
  FILES "${CMAKE_CURRENT_BINARY_DIR}/${METAINFO_ID}.metainfo.xml"
  DESTINATION "${CMAKE_INSTALL_DATADIR}/metainfo"
  COMPONENT qt6gui
)
install(
  FILES celestia-qt6.1
  DESTINATION "${CMAKE_INSTALL_DATADIR}/man/man1"
  COMPONENT qt6gui
)
