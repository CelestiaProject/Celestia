set(CELFS_SOURCES
  fs.cpp
  fs.h
)

add_library(celfs STATIC ${CELFS_SOURCES})
cotire(celfs)
