set(CELESTIA_LIBS
  cel3ds
  celengine
  celephem
  celestia
  celmath
  celmodel
  celtxf
  celutil
)

foreach(lib ${CELESTIA_LIBS} tools)
  add_subdirectory(${lib})
endforeach()

add_executable(buildstardb buildstardb.cpp)
cotire(buildstardb)
