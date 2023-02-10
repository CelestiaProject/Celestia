install(FILES celestiaui.xml
        DESTINATION "${DATADIR}")
install(FILES celestia-gtk.desktop
        DESTINATION "${CMAKE_INSTALL_DATADIR}/applications")
install(FILES space.celestia.celestia_gtk.metainfo.xml
        DESTINATION "${CMAKE_INSTALL_DATADIR}/metainfo")
install(FILES celestia-gtk.1
        DESTINATION "${CMAKE_INSTALL_DATADIR}/man/man1")
