set(CELUTIL_SOURCES
  bigfix.cpp
  bigfix.h
  binaryread.h
  binarywrite.h
  blockarray.h
  bytes.h
  color.cpp
  color.h
  filetype.cpp
  filetype.h
  formatnum.cpp
  formatnum.h
  fsutils.cpp
  fsutils.h
  logger.cpp
  logger.h
  reshandle.h
  resmanager.h
  stringutils.cpp
  stringutils.h
  strnatcmp.cpp
  strnatcmp.h
  timer.cpp
  timer.h
  tokenizer.cpp
  tokenizer.h
  tzutil.cpp
  tzutil.h
  utf8.cpp
  utf8.h
  watcher.h
)

if (WIN32)
  list(APPEND CELUTIL_SOURCES
    winutil.cpp
    winutil.h
  )
endif()

if (APPLE)
  list(APPEND CELUTIL_SOURCES
    appleutils.h
    appleutils.mm
  )
endif()

add_library(celutil OBJECT ${CELUTIL_SOURCES})
