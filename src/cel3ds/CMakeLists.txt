set(CEL3DS_SOURCES
  3dschunk.h
  3dsmodel.cpp
  3dsmodel.h
  3dsread.cpp
  3dsread.h
)

add_library(cel3ds OBJECT ${CEL3DS_SOURCES})
cotire(cel3ds)
