set(CELTFX_SOURCES
  texturefont.cpp
  texturefont.h
)

add_library(celtxf STATIC ${CELTFX_SOURCES})
cotire(celtxf)
#[[
add_library(celtxf SHARED ${CELTFX_SOURCES})
set_target_properties(celtxf PROPERTIES
  VERSION ${VERSION}
  SOVERSION ${VERSION}
)
]]#
#install(TARGETS celtxf LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})
install(TARGETS celtxf ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})
