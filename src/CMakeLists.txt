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

foreach(lib ${CELESTIA_LIBS})
  add_subdirectory(${lib})
endforeach()

if (ENABLE_TOOLS)
  add_subdirectory(tools)
endif()
