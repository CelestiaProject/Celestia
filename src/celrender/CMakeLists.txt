set(CELRENDER_SOURCES
  atmosphererenderer.cpp
  atmosphererenderer.h
  cometrenderer.cpp
  cometrenderer.h
  eclipticlinerenderer.cpp
  eclipticlinerenderer.h
  linerenderer.cpp
  linerenderer.h
  vertexobject.cpp
  vertexobject.h
)

add_library(celrender OBJECT ${CELRENDER_SOURCES})
