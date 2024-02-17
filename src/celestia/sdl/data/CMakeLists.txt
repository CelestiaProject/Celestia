set(METAINFO_ID "space.celestiaproject.Celestia-sdl")
set(METAINFO_NAME "Celestia (SDL)")
set(METAINFO_DESKTOP "celestia-sdl.desktop")
configure_file("../../data/space.celestiaproject.Celestia.metainfo.xml.in" "${METAINFO_ID}.metainfo.xml")

install(
  FILES "${METAINFO_DESKTOP}"
  DESTINATION "${CMAKE_INSTALL_DATADIR}/applications"
  COMPONENT sdlgui
)
install(
  FILES "${METAINFO_ID}.metainfo.xml"
  DESTINATION "${CMAKE_INSTALL_DATADIR}/metainfo"
  COMPONENT sdlgui
)
install(
  FILES celestia-sdl.1
  DESTINATION "${CMAKE_INSTALL_DATADIR}/man/man1"
  COMPONENT sdlgui
)
