set(CELESTIA_LIBS
  cel3ds
  celengine
  celestia
  celmath
  celmodel
  celutil
)

foreach(lib ${CELESTIA_LIBS})
  add_subdirectory(${lib})
endforeach()

add_subdirectory(celscript)

# These compiled objects are merged with the celengine library
add_subdirectory(celephem)
add_subdirectory(celcompat)

if (ENABLE_TTF)
  add_subdirectory(celttf)
else()
  add_subdirectory(celtxf)
endif()

if (ENABLE_TOOLS)
  add_subdirectory(tools)
endif()
