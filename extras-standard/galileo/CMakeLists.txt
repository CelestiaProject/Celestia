set(GALILEO_SOURCES
  galileo.ssc
  data/galileo-orbit.xyzv
  data/galileo-cruise.xyzv
  models/galileo.3ds
)

install_to_extras_subdir("${DATADIR}" galileo ${GALILEO_SOURCES})
