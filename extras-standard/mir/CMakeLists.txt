set(MIR_SOURCES
  models/mir.3ds
  mir.ssc
)

foreach ( file ${MIR_SOURCES} )
    get_filename_component( dir ${file} DIRECTORY )
    install( FILES ${file} DESTINATION "${DATADIR}/extras-standard/mir/${dir}" )
  endforeach()
