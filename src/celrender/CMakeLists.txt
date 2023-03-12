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
  largestarrenderer.cpp
  largestarrenderer.h
  linerenderer.cpp
  linerenderer.h
  gl/buffer.cpp
  gl/buffer.h
  gl/vertexobject.cpp
  gl/vertexobject.h
)

add_library(celrender OBJECT ${CELRENDER_SOURCES})
