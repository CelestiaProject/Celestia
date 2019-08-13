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

# Compiled celephem objects are merged with the celengine library
add_subdirectory(celephem)

if (ENABLE_TOOLS)
  add_subdirectory(tools)
endif()

if (NOT HAVE_EXPERIMENTAL_FILESYSTEM)
  add_subdirectory(celfs)
endif()
