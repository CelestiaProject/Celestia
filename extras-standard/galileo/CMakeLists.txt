set(GALILEO_SOURCES
  galileo.ssc
  data/galileo-orbit.xyzv
  data/galileo-cruise.xyzv
  models/galileo.3ds
)

foreach ( file ${GALILEO_SOURCES} )
    get_filename_component( dir ${file} DIRECTORY )
    install( FILES ${file} DESTINATION "${DATADIR}/extras-standard/galileo/${dir}" )
  endforeach()
