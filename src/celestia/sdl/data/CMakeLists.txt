install(
  FILES celestia-sdl.desktop
  DESTINATION "${CMAKE_INSTALL_DATADIR}/applications"
  COMPONENT sdlgui
)
install(
  FILES space.celestia.celestia_sdl.metainfo.xml
  DESTINATION "${CMAKE_INSTALL_DATADIR}/metainfo"
  COMPONENT sdlgui
)
install(
  FILES celestia-sdl.1
  DESTINATION "${CMAKE_INSTALL_DATADIR}/man/man1"
  COMPONENT sdlgui
)
