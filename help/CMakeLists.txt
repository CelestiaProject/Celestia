file(GLOB HELP_SOURCES "*.html")
file(GLOB HELP2_SOURCES "CelestiaGuide/*.*")

install(
  FILES ${HELP_SOURCES}
  DESTINATION "${DATADIR}/help"
  COMPONENT core
)
install(
  FILES ${HELP2_SOURCES}
  DESTINATION "${DATADIR}/help/CelestiaGuide"
  COMPONENT core
)
