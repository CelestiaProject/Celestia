file(GLOB MEDRES_SOURCES "*.jpg" "*.png" "*.dds")

install(FILES ${MEDRES_SOURCES} DESTINATION "${DATADIR}/textures/medres")
