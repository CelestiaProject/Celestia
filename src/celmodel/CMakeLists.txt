set(CELMODEL_SOURCES
  material.h
  mesh.cpp
  mesh.h
  model.cpp
  modelfile.cpp
  modelfile.h
  model.h
  tangents.cpp
)

add_library(celmodel OBJECT ${CELMODEL_SOURCES})
