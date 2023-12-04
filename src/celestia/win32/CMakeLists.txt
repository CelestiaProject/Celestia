if(NOT ((WIN32 OR MINGW) AND ENABLE_WIN))
  message(STATUS "Either not Windows or Windows frontend is disabled.")
  return()
endif()

set(WIN32_SOURCES
  datetimehelpers.cpp
  odmenu.cpp
  tstring.cpp
  winbookmarks.cpp
  wincontextmenu.cpp
  windatepicker.cpp
  windisplaymodedlg.cpp
  windroptarget.cpp
  wineclipses.cpp
  winfiledlgs.cpp
  winfinddlg.cpp
  wingotodlg.cpp
  winhelpdlgs.cpp
  winhyperlinks.cpp
  winlocations.cpp
  winmain.cpp
  winmainwindow.cpp
  winpreferences.cpp
  winsplash.cpp
  winssbrowser.cpp
  winstarbrowser.cpp
  wintime.cpp
  wintourguide.cpp
  winuiutils.cpp
  winviewoptsdlg.cpp
)

set(WIN32_HEADERS
  datetimehelpers.h
  odmenu.h
  tcharconv.h
  tstring.h
  winbookmarks.h
  wincontextmenu.h
  windatepicker.h
  windisplaymodedlg.h
  windroptarget.h
  wineclipses.h
  winfiledlgs.h
  winfinddlg.h
  wingotodlg.h
  winhelpdlgs.h
  winhyperlinks.h
  winlocations.h
  winmainwindow.h
  winpreferences.h
  winsplash.h
  winssbrowser.h
  winstarbrowser.h
  wintime.h
  wintourguide.h
  winuiutils.h
  winviewoptsdlg.h
)

if(ENABLE_FFMPEG)
  list(APPEND WIN32_SOURCES winmoviecapture.cpp)
  list(APPEND WIN32_HEADERS winmoviecapture.h)
endif()

set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /MANIFEST:NO")

add_library(resources OBJECT res/celestia.rc)
target_include_directories(resources PRIVATE "${CMAKE_SOURCE_DIR}/src/celestia/win32/res")
set(RESOURCES $<TARGET_OBJECTS:resources>)

add_executable(celestia-win WIN32 ${WIN32_SOURCES} ${RESOURCES})
add_dependencies(celestia-win celestia)
target_link_libraries(celestia-win celestia ${OPENGL_LIBRARIES})
target_include_directories(celestia-win PRIVATE ${OPENGL_INCLUDE_DIRS})

set_target_properties(celestia-win PROPERTIES CXX_VISIBILITY_PRESET hidden)

install(
  TARGETS celestia-win
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
  COMPONENT wingui
)

add_subdirectory(res)
