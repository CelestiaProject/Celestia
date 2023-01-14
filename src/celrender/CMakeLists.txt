set(CELRENDER_SOURCES
  cometrenderer.cpp
  cometrenderer.h
  linerenderer.cpp
  linerenderer.h
  vertexobject.cpp
  vertexobject.h
)

add_library(celrender OBJECT ${CELRENDER_SOURCES})
