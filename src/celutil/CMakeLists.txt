set(CELUTIL_SOURCES
  bigfix.cpp
  bigfix.h
  blockarray.h
  bytes.h
  color.cpp
  color.h
  debug.cpp
  debug.h
  filetype.cpp
  filetype.h
  formatnum.cpp
  formatnum.h
  #memorypool.cpp
  #memorypool.h
  packedrangeset.h
  reshandle.h
  resmanager.h
  timer.cpp
  timer.h
  utf8.cpp
  utf8.h
  util.cpp
  util.h
  watcher.h
)

if (WIN32)
  list(APPEND CELUTIL_SOURCES
    winutil.cpp
    winutil.h
  )
endif()

add_library(celutil STATIC ${CELUTIL_SOURCES} $<TARGET_OBJECTS:celcompat>)

#[[
add_library(celutil SHARED ${CELUTIL_SOURCES})
set_target_properties(celutil PROPERTIES
  VERSION ${VERSION}
  SOVERSION ${VERSION}
)
]]#

add_dependencies(celutil celmath)
target_link_libraries(celutil celmath)

cotire(celutil)

#install(TARGETS celutil LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})
