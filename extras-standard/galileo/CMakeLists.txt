set(GALILEO_SOURCES
  galileo.ssc
  data/galileo-orbit.xyzv
  data/galileo-cruise.xyzv
  models/galileo.3ds
)

install(FILES ${GALILEO_SOURCES} DESTINATION "${DATADIR}/extras-standard/galileo")
