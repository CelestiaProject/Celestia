if(NOT ((WIN32 OR MINGW) AND ENABLE_WIN))
  message(STATUS "Either not Windows or Windows frontend is disabled.")
  return()
endif()

set(WIN32_SOURCES
  odmenu.cpp
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
  winuiutils.cpp
  winviewoptsdlg.cpp
)

set(WIN32_HEADERS
  odmenu.h
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
  winuiutils.h
  winviewoptsdlg.h
)

set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /MANIFEST:NO")

add_library(resources OBJECT res/celestia.rc)
target_include_directories(resources PRIVATE "${CMAKE_SOURCE_DIR}/src/celestia/win32/res")
set(RESOURCES $<TARGET_OBJECTS:resources>)

add_executable(celestia-win WIN32 ${WIN32_SOURCES} ${RESOURCES})
add_dependencies(celestia-win celestia)
target_link_libraries(celestia-win celestia ${OPENGL_LIBRARIES})
target_include_directories(celestia-win PRIVATE ${OPENGL_INCLUDE_DIRS})
install(TARGETS celestia-win RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

add_subdirectory(res)
