set(CELESTIA_LIBS
  cel3ds
  celcompat
  celengine
  celephem
  celestia
  celimage
  celmath
  celmodel
  celrender
  celttf
  celutil
)

foreach(lib ${CELESTIA_LIBS})
  add_subdirectory(${lib})
endforeach()

add_subdirectory(celscript)

if (ENABLE_TOOLS)
  add_subdirectory(tools)
endif()
