if(NOT HAVE_FLOAT_CHARCONV)
set(CELCOMPAT_SOURCES
  charconv_impl.cpp
  charconv_impl.h
)
endif()

if(CELCOMPAT_SOURCES)
  add_library(celcompat OBJECT ${CELCOMPAT_SOURCES})
  if(NOT MSVC)
    target_compile_options(celcompat PRIVATE -fmath-errno)
  endif()
endif()
