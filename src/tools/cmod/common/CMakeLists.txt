set(CMODCOMMON_SOURCES
  cmodops.cpp
  cmodops.h
  convert3ds.cpp
  convert3ds.h
  convertobj.cpp
  convertobj.h
)

add_library(cmodcommon STATIC ${CMODCOMMON_SOURCES})
cotire(cmodcommon)
