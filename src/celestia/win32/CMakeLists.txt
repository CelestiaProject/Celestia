if(NOT WIN32 OR NOT ENABLE_WIN)
  message("Either not Windows or Windows frontend is disabled.")
  return()
endif()

set(WIN32_SOURCES
  ODMenu.cpp
  wglext.cpp
  winbookmarks.cpp
  windatepicker.cpp
  wineclipses.cpp
  wingotodlg.cpp
  winhyperlinks.cpp
  winlocations.cpp
  winmain.cpp
  winsplash.cpp
  winssbrowser.cpp
  winstarbrowser.cpp
  wintime.cpp
  wintourguide.cpp
  winviewoptsdlg.cpp
)

set(WIN32_HEADERS
  ODMenu.h
  wglext.h
  winbookmarks.h
  wineclipses.h
  wingotodlg.h
  winhyperlinks.h
  winlocations.h
  winsplash.h
  winssbrowser.h
  winstarbrowser.h
  wintime.h
  wintourguide.h
  winviewoptsdlg.h
)

add_library(resources OBJECT res/celestia.rc)
target_include_directories(resources PRIVATE "${CMAKE_SOURCE_DIR}/src/celestia/win32/res")
set(RESOURCES $<TARGET_OBJECTS:resources>)

add_executable(celestia-win WIN32 ${WIN32_SOURCES} ${RESOURCES})
cotire(celestia-win)
target_link_libraries(celestia-win ${CELESTIA_LIBS})
install(TARGETS celestia-win RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
