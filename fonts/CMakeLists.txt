file(GLOB FONTS_SOURCES "*.txf" "*.ttf")

install(FILES ${FONTS_SOURCES} DESTINATION "${DATADIR}/fonts")
