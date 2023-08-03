install(
  FILES celestiaui.xml celestia-logo.png
  DESTINATION "${DATADIR}"
  COMPONENT gtkgui
)
install(
  FILES celestia-gtk.desktop
  DESTINATION "${CMAKE_INSTALL_DATADIR}/applications"
  COMPONENT gtkgui
)
install(
  FILES space.celestia.celestia_gtk.metainfo.xml
  DESTINATION "${CMAKE_INSTALL_DATADIR}/metainfo"
  COMPONENT gtkgui
)
install(
  FILES celestia-gtk.1
  DESTINATION "${CMAKE_INSTALL_DATADIR}/man/man1"
  COMPONENT gtkgui
)
