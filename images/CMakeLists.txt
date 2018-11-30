file(GLOB IMAGES_SOURCES INFO.txt "*.jpg" "*.png" "*.dds")

install(FILES ${IMAGES_SOURCES} DESTINATION "${DATADIR}/images")
