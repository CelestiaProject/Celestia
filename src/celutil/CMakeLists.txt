set(CELUTIL_SOURCES
  bigfix.cpp
  bigfix.h
  bytes.h
  color.cpp
  color.h
  debug.cpp
  debug.h
  directory.cpp
  directory.h
  filetype.cpp
  filetype.h
  formatnum.cpp
  formatnum.h
  #memorypool.cpp
  #memorypool.h
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
    windirectory.cpp
    winutil.cpp
    winutil.h
  )
else()
  list(APPEND CELUTIL_SOURCES
    unixdirectory.cpp
  )
endif()

add_library(celutil STATIC ${CELUTIL_SOURCES})

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
