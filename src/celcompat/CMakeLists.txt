if (NOT HAVE_FLOAT_CHARCONV)
set(CELCOMPAT_SOURCES
  cc.cpp
  cc.h
)
endif()

if (CELCOMPAT_SOURCES)
  add_library(celcompat OBJECT ${CELCOMPAT_SOURCES})
endif()
