set(GALILEO_SOURCES
  dwarfplanets.ssc
  data/Ceres-Dawn.xyzv
)

install_to_extras_subdir("${DATADIR}" dwarf-planets ${GALILEO_SOURCES})
