set(CELESTIA_LIBS
  cel3ds
  celengine
  celestia
  celmath
  celmodel
  celtxf
  celutil
)

foreach(lib ${CELESTIA_LIBS})
  add_subdirectory(${lib})
endforeach()

# These compiled objects are merged with the celengine library
add_subdirectory(celephem)
add_subdirectory(celcompat)

if (ENABLE_TOOLS)
  add_subdirectory(tools)
endif()
