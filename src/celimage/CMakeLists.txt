set(CELIMAGE_SOURCES
  avif.cpp
  bmp.cpp
  dds.cpp
  dds_decompress.cpp
  dds_decompress.h
  imageformats.h
  jpeg.cpp
  png.cpp
)

add_library(celimage OBJECT ${CELIMAGE_SOURCES})
