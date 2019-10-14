set(LEGACY_SOURCES
  cmdparser.cpp
  cmdparser.h
  command.cpp
  command.h
  execution.cpp
  execution.h
)

add_library(cellegacyscript OBJECT ${LEGACY_SOURCES})
