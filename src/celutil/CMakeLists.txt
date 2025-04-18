set(CELUTIL_SOURCES
  align.h
  array_view.h
  associativearray.cpp
  associativearray.h
  binaryread.h
  binarywrite.h
  blockarray.h
  bytes.h
  color.cpp
  color.h
  dateformatter.cpp
  dateformatter.h
  filetype.cpp
  filetype.h
  flag.h
  formatnum.cpp
  formatnum.h
  fsutils.cpp
  fsutils.h
  greek.cpp
  greek.h
  includeicu.h
  infourl.cpp
  infourl.h
  localeutil.h
  logger.cpp
  logger.h
  parser.cpp
  parser.h
  ranges.h
  r128.h
  r128util.cpp
  r128util.h
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
  uniquedel.h
  utf8.cpp
  utf8.h
  watcher.h
)

if (USE_ICU)
  list(APPEND CELUTIL_SOURCES
    unicode.cpp
    unicode.h
  )
endif()

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

gperf_add_table(celutil parser.gperf parser.cpp 4)
