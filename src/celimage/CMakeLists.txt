set(CELIMAGE_SOURCES
  bmp.cpp
  dds.cpp
  dds_decompress.cpp
  dds_decompress.h
  image.cpp
  image.h
  imageformats.h
  jpeg.cpp
  pixelformat.h
  png.cpp
)

if(ENABLE_LIBAVIF)
  list(APPEND CELIMAGE_SOURCES
    avif.cpp
  )
endif()

add_library(celimage OBJECT ${CELIMAGE_SOURCES})
