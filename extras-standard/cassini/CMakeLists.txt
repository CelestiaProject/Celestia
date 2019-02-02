set(CASSINI_SOURCES
  data/cassini-cruise.xyzv
  data/cassini-solstice.xyzv
  data/huygens.xyzv
  data/cassini-orbit.xyzv
  cassini.ssc
  models/cassini.3ds
  models/huygens.3ds
)

foreach ( file ${CASSINI_SOURCES} )
    get_filename_component( dir ${file} DIRECTORY )
    install( FILES ${file} DESTINATION "${DATADIR}/extras-standard/cassini/${dir}" )
  endforeach()
