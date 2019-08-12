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

add_library(celmodel STATIC ${CELMODEL_SOURCES})

#[[
add_library(celmodel SHARED ${CELMODEL_SOURCES})
set_target_properties(celmodel PROPERTIES
  VERSION ${VERSION}
  SOVERSION ${VERSION}
)
]]#

add_dependencies(celmodel celutil)
target_link_libraries(celmodel celutil)

cotire(celmodel)

#install(TARGETS celmodel LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})
#install(TARGETS celmodel ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})
