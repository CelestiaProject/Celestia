set(SCRIPT_COMMON_SOURCES
  script.cpp
  script.h
  scriptmaps.cpp
  scriptmaps.h
)

add_library(celcommonscript OBJECT ${SCRIPT_COMMON_SOURCES})
