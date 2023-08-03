install(
  FILES celestia-qt5.desktop
  DESTINATION "${CMAKE_INSTALL_DATADIR}/applications"
  COMPONENT qt5gui
)
install(
  FILES space.celestia.celestia_qt5.metainfo.xml
  DESTINATION "${CMAKE_INSTALL_DATADIR}/metainfo"
  COMPONENT qt5gui
)
install(
  FILES celestia-qt5.1
  DESTINATION "${CMAKE_INSTALL_DATADIR}/man/man1"
  COMPONENT qt5gui
)
