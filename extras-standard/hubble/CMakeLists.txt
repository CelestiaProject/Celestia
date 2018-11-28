set(HUBBLE_SOURCES
  hubble.ssc
  models/hubble.cmod
)

install(FILES ${HUBBLE_SOURCES} DESTINATION "${DATADIR}/extras-standard/hubble")
