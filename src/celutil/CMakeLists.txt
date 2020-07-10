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

add_library(celutil OBJECT ${CELUTIL_SOURCES})
cotire(celutil)
