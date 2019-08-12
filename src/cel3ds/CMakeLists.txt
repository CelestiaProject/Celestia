set(CEL3DS_SOURCES
  3dschunk.h
  3dsmodel.cpp
  3dsmodel.h
  3dsread.cpp
  3dsread.h
)

add_library(cel3ds STATIC ${CEL3DS_SOURCES})

#[[
add_library(cel3ds SHARED ${CEL3DS_SOURCES})
set_target_properties(cel3ds PROPERTIES
  VERSION ${VERSION}
  SOVERSION ${VERSION}
)
]]#

add_dependencies(cel3ds celutil)
target_link_libraries(cel3ds celutil)

cotire(cel3ds)

#install(TARGETS cel3ds LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})
