set(LEGACY_SOURCES
  cmdparser.cpp
  cmdparser.h
  command.cpp
  command.h
  execenv.h
  execution.cpp
  execution.h
  legacyscript.cpp
  legacyscript.h
)

add_library(cellegacyscript OBJECT ${LEGACY_SOURCES})

gperf_add_table(cellegacyscript commands.gperf cmdparser.cpp 4)
