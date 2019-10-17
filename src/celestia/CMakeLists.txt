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
  helper.cpp
  helper.h
  imagecapture.cpp
  imagecapture.h
  moviecapture.h
  scriptmenu.cpp
  scriptmenu.h
  url.cpp
  url.h
  view.cpp
  view.h
)

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

add_library(celestia STATIC ${CELESTIA_SOURCES}
                            $<TARGET_OBJECTS:celcommonscript>
                            $<TARGET_OBJECTS:cellegacyscript>
                            $<TARGET_OBJECTS:celluascript>)

#[[
add_library(celestia SHARED ${CELESTIA_SOURCES})
set_target_properties(celestia PROPERTIES
  VERSION ${VERSION}
  SOVERSION ${VERSION}
)
]]#

add_dependencies(celestia celengine celmath celtxf celutil)
target_link_libraries(celestia celengine celmath celtxf celutil)

cotire(celestia)

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
