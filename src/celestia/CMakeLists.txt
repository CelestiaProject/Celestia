set(CELESTIA_SOURCES
  celestiacore.cpp
  celestiacore.h
  celestiastate.cpp
  celestiastate.h
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
elseif(ENABLE_FFMPEG)
  list(APPEND CELESTIA_SOURCES
    ffmpegcapture.cpp
    ffmpegcapture.h
  )
endif()

set(SCRIPT_LIBS $<TARGET_OBJECTS:celcommonscript> $<TARGET_OBJECTS:cellegacyscript>)
if(ENABLE_CELX)
  set(SCRIPT_LIBS ${SCRIPT_LIBS} $<TARGET_OBJECTS:celluascript>)
endif()

string(REPLACE "." ";" PROJECT_VERSION_LIST ${PROJECT_VERSION})
list(GET PROJECT_VERSION_LIST 0 PROJECT_VERSION_MAJOR)
list(GET PROJECT_VERSION_LIST 1 PROJECT_VERSION_MINOR)
set(CELSO "celestia-${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}")

add_library(celestia SHARED ${CELESTIA_SOURCES}
                            ${SCRIPT_LIBS}
                            $<TARGET_OBJECTS:cel3ds>
                            $<TARGET_OBJECTS:celcompat>
                            $<TARGET_OBJECTS:celengine>
                            $<TARGET_OBJECTS:celephem>
                            $<TARGET_OBJECTS:celimage>
                            $<TARGET_OBJECTS:celmath>
                            $<TARGET_OBJECTS:celmodel>
                            $<TARGET_OBJECTS:celttf>
                            $<TARGET_OBJECTS:celutil>
)

set_target_properties(celestia PROPERTIES
  VERSION ${PROJECT_VERSION}
  SOVERSION ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
  WINDOWS_EXPORT_ALL_SYMBOLS TRUE
)

if(ENABLE_SPICE)
  target_link_libraries(celestia CSPICE::CSPICE)
endif()

if(APPLE)
  target_link_libraries(celestia "-framework Foundation")
endif()

if(ENABLE_FFMPEG)
  target_link_libraries(celestia ${FFMPEG_LIBRARIES})
endif()

install(TARGETS celestia LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR} NAMELINK_SKIP)

add_subdirectory(glut)
add_subdirectory(gtk)
add_subdirectory(qt)
add_subdirectory(sdl)
add_subdirectory(win32)
