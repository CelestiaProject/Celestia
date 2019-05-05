set(CELTFX_SOURCES
  texturefont.cpp
  texturefont.h
)

add_library(celtxf STATIC ${CELTFX_SOURCES})

#[[
add_library(celtxf SHARED ${CELTFX_SOURCES})
set_target_properties(celtxf PROPERTIES
  VERSION ${VERSION}
  SOVERSION ${VERSION}
)
]]#

add_dependencies(celtxf celutil)
target_link_libraries(celtxf celutil)

#install(TARGETS celtxf LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})
