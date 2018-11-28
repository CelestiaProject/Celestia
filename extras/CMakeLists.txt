set(EXTRA_SOURCES
  apollo.ssc
  shroxclassic.ssc
  shroxmars.ssc
  skylab.ssc
)

install(FILES ${EXTRA_SOURCES} DESTINATION "${DATADIR}/extras")
