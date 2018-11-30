set(CELMATH_SOURCES
  distance.h
  ellipsoid.h
  frustum.cpp
  frustum.h
  geomutil.h
  intersect.h
  mathlib.h
  perlin.cpp
  perlin.h
  ray.h
  solve.h
  sphere.h
)

add_library(celmath STATIC ${CELMATH_SOURCES})
cotire(celmath)
#[[
add_library(celmath SHARED ${CELMATH_SOURCES})
set_target_properties(celmath PROPERTIES
  VERSION ${VERSION}
  SOVERSION ${VERSION}
)
]]#
#install(TARGETS celmath LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})
