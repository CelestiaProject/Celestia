set(CELEPHEM_SOURCES
  customorbit.cpp
  customorbit.h
  customrotation.cpp
  customrotation.h
  jpleph.cpp
  jpleph.h
  nutation.cpp
  nutation.h
  orbit.cpp
  orbit.h
  precession.cpp
  precession.h
  rotation.cpp
  rotation.h
  samporbit.cpp
  samporbit.h
  samporient.cpp
  samporient.h
  vsop87.cpp
  vsop87.h
)

if(ENABLE_SPICE)
  list(APPEND CELEPHEM_SOURCES
    spiceinterface.cpp
    spiceinterface.h
    spiceorbit.cpp
    spiceorbit.h
    spicerotation.cpp
    spicerotation.h
  )
endif()


if(ENABLE_CELX)
  list(APPEND CELEPHEM_SOURCES
    scriptobject.cpp
    scriptobject.h
    scriptorbit.cpp
    scriptorbit.h
    scriptrotation.cpp
    scriptrotation.h
  )
endif()

# These object files are merged in the celegine library
add_library(celephem OBJECT ${CELEPHEM_SOURCES})
