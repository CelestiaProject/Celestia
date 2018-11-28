file(GLOB LOCALE_SOURCES "controls_*.txt" "*.cel")

install(FILES ${LOCALE_SOURCES} DESTINATION "${DATADIR}")
