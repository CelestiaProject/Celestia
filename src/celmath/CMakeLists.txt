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

add_library(celmath OBJECT ${CELMATH_SOURCES})
