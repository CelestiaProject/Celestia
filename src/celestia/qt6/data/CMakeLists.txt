install(
  FILES celestia-qt6.desktop
  DESTINATION "${CMAKE_INSTALL_DATADIR}/applications"
  COMPONENT qt6gui
)
install(
  FILES space.celestia.celestia_qt6.metainfo.xml
  DESTINATION "${CMAKE_INSTALL_DATADIR}/metainfo"
  COMPONENT qt6gui
)
install(
  FILES celestia-qt6.1
  DESTINATION "${CMAKE_INSTALL_DATADIR}/man/man1"
  COMPONENT qt6gui
)
