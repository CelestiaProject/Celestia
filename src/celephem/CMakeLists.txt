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

add_library(celephem STATIC ${CELEPHEM_SOURCES})
if(ENABLE_SPICE)
  target_link_libraries(celephem CSPICE::CSPICE CSPICE::CSUPPORT)
endif()
cotire(celephem)
#[[
add_library(celephem SHARED ${CELEPHEM_SOURCES})
set_target_properties(celephem PROPERTIES
  VERSION ${VERSION}
  SOVERSION ${VERSION}
)
]]#
#install(TARGETS celephem LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})
