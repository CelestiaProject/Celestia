set(CEL3DS_SOURCES
  3dschunk.h
  3dsmodel.cpp
  3dsmodel.h
  3dsread.cpp
  3dsread.h
)

add_library(cel3ds STATIC ${CEL3DS_SOURCES})
cotire(cel3ds)
#[[
add_library(cel3ds SHARED ${CEL3DS_SOURCES})
set_target_properties(cel3ds PROPERTIES
  VERSION ${VERSION}
  SOVERSION ${VERSION}
)
]]#
#install(TARGETS cel3ds LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})
