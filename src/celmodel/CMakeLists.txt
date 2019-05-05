set(CELMODEL_SOURCES
  material.cpp
  material.h
  mesh.cpp
  mesh.h
  model.cpp
  modelfile.cpp
  modelfile.h
  model.h
)

add_library(celmodel OBJECT ${CELMODEL_SOURCES})
