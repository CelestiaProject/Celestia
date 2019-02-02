set(HUBBLE_SOURCES
  hubble.ssc
  models/hubble.cmod
)

foreach ( file ${HUBBLE_SOURCES} )
    get_filename_component( dir ${file} DIRECTORY )
    install( FILES ${file} DESTINATION "${DATADIR}/extras-standard/hubble/${dir}" )
  endforeach()
