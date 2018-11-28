file(GLOB LORES_SOURCES "*.jpg" "*.png" "*.dds")

install(FILES ${LORES_SOURCES} DESTINATION "${DATADIR}/textures/lores")
