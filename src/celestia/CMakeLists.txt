set(CELESTIA_SOURCES
  celestiacore.cpp
  celestiacore.h
  configfile.cpp
  configfile.h
  destination.cpp
  destination.h
  eclipsefinder.cpp
  eclipsefinder.h
  favorites.cpp
  favorites.h
  imagecapture.cpp
  imagecapture.h
  moviecapture.h
  scriptmenu.cpp
  scriptmenu.h
  url.cpp
  url.h
)

set(CELX_SOURCES
  celx_category.cpp
  celx_category.h
  celx_celestia.cpp
  celx_celestia.h
  celx.cpp
  celx_frame.cpp
  celx_frame.h
  celx_gl.cpp
  celx_gl.h
  celx.h
  celx_internal.h
  celx_misc.cpp
  celx_misc.h
  celx_object.cpp
  celx_object.h
  celx_observer.cpp
  celx_observer.h
  celx_phase.cpp
  celx_phase.h
  celx_position.cpp
  celx_position.h
  celx_rotation.cpp
  celx_rotation.h
  celx_vector.cpp
  celx_vector.h
)

if(ENABLE_CELX)
  list(APPEND CELESTIA_SOURCES ${CELX_SOURCES})
endif()

if(WIN32)
  list(APPEND CELESTIA_SOURCES
    avicapture.cpp
    avicapture.h
  )
elseif(_UNIX AND ENABLE_THEORA)
  list(APPEND CELESTIA_SOURCES
    oggtheoracapture.cpp
    oggtheoracapture.h
  )
endif()

if(NOT fmt_FOUND)
  list(APPEND CELESTIA_SOURCES
    ${CMAKE_SOURCE_DIR}/thirdparty/fmt/src/format.cc
    ${CMAKE_SOURCE_DIR}/thirdparty/fmt/src/posix.cc
  )
endif()

add_library(celestia STATIC ${CELESTIA_SOURCES})
#[[
add_library(celestia SHARED ${CELESTIA_SOURCES})
set_target_properties(celestia PROPERTIES
  VERSION ${VERSION}
  SOVERSION ${VERSION}
)
]]#
add_dependencies(celestia celengine celephem)
cotire(celestia)
target_link_libraries(celestia celengine celephem)
#install(TARGETS celestia LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})

# celestia-glut binary
if(ENABLE_GLUT)
  find_package(GLUT)
  if(NOT GLUT_FOUND)
    message(WARNING "GLUT library isn't found, not building GLUT fronend.")
  else()
    set(GLUT_SOURCES glutmain.cpp)
    add_executable(celestia-glut ${GLUT_SOURCES})
    cotire(celestia-glut)
    target_include_directories(celestia-glut PRIVATE ${GLUT_INCLUDE_DIR})
    target_link_libraries(celestia-glut ${CELESTIA_LIBS} ${GLUT_LIBRARIES})
    install(TARGETS celestia-glut RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
  endif()
endif()

add_subdirectory(gtk)
add_subdirectory(qt)
add_subdirectory(win32)
