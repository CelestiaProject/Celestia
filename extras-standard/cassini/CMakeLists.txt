set(CASSINI_SOURCES
  data/cassini-cruise.xyzv
  data/cassini-solstice.xyzv
  data/huygens.xyzv
  data/cassini-orbit.xyzv
  cassini.ssc
  models/cassini.3ds
  models/huygens.3ds
)

install(FILES ${CASSINI_SOURCES} DESTINATION "${DATADIR}/extras-standard/cassini")
