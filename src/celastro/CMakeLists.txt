set(CELASTRO_SOURCES
  astro.cpp
  astro.h
  date.cpp
  date.h
  units.cpp
  units.h)

add_library(celastro OBJECT ${CELASTRO_SOURCES})
