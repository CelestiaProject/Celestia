file(GLOB HIRES_SOURCES "*.jpg" "*.png" "*.dds")

install(FILES ${HIRES_SOURCES} DESTINATION "${DATADIR}/textures/hires")
