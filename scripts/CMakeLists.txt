file(GLOB SCRIPTS_SOURCES "*.cel" "*.celx")

install(FILES ${SCRIPTS_SOURCES} DESTINATION "${DATADIR}/scripts")
