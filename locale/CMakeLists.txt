file(GLOB LOCALE_SOURCES "controls_*.txt" "COPYING_*" "*.cel")

install(FILES ${LOCALE_SOURCES} DESTINATION "${DATADIR}/locale")
