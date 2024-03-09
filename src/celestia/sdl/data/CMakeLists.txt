set(METAINFO_ID "space.celestiaproject.celestia_sdl")
set(METAINFO_NAME "Celestia (SDL)")
set(METAINFO_DESKTOP "space.celestiaproject.celestia_sdl.desktop")
configure_file(
  "../../data/space.celestiaproject.Celestia.metainfo.xml.in"
  "${CMAKE_CURRENT_BINARY_DIR}/${METAINFO_ID}.metainfo.xml"
)

install(
  FILES "${METAINFO_DESKTOP}"
  DESTINATION "${CMAKE_INSTALL_DATADIR}/applications"
  COMPONENT sdlgui
)
install(
  FILES "${CMAKE_CURRENT_BINARY_DIR}/${METAINFO_ID}.metainfo.xml"
  DESTINATION "${CMAKE_INSTALL_DATADIR}/metainfo"
  COMPONENT sdlgui
)
install(
  FILES celestia-sdl.1
  DESTINATION "${CMAKE_INSTALL_DATADIR}/man/man1"
  COMPONENT sdlgui
)
