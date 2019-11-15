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

set(SCRIPT_LIBS $<TARGET_OBJECTS:celcommonscript> $<TARGET_OBJECTS:cellegacyscript>)
if(ENABLE_CELX)
  set(SCRIPT_LIBS ${SCRIPT_LIBS} $<TARGET_OBJECTS:celluascript>)
endif()

add_library(celestia STATIC ${CELESTIA_SOURCES} ${SCRIPT_LIBS})

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

add_subdirectory(glut)
add_subdirectory(gtk)
add_subdirectory(qt)
add_subdirectory(win32)
