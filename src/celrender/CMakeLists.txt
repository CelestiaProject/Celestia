set(CELRENDER_SOURCES
  asterismrenderer.cpp
  asterismrenderer.h
  atmosphererenderer.cpp
  atmosphererenderer.h
  boundariesrenderer.cpp
  boundariesrenderer.h
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
