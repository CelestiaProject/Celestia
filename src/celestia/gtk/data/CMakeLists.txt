set(METAINFO_ID "space.celestiaproject.celestia_gtk")
set(METAINFO_NAME "Celestia (GTK)")
set(METAINFO_DESKTOP "space.celestiaproject.celestia_gtk.desktop")
configure_file(
  "../../data/space.celestiaproject.Celestia.metainfo.xml.in"
  "${CMAKE_CURRENT_BINARY_DIR}/${METAINFO_ID}.metainfo.xml"
)

install(
  FILES celestiaui.xml celestia-logo.png
  DESTINATION "${DATADIR}"
  COMPONENT gtkgui
)
install(
  FILES "${METAINFO_DESKTOP}"
  DESTINATION "${CMAKE_INSTALL_DATADIR}/applications"
  COMPONENT gtkgui
)
install(
  FILES "${CMAKE_CURRENT_BINARY_DIR}/${METAINFO_ID}.metainfo.xml"
  DESTINATION "${CMAKE_INSTALL_DATADIR}/metainfo"
  COMPONENT gtkgui
)
install(
  FILES celestia-gtk.1
  DESTINATION "${CMAKE_INSTALL_DATADIR}/man/man1"
  COMPONENT gtkgui
)
