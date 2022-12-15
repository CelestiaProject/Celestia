if(NOT _UNIX OR NOT ENABLE_GTK)
  message(STATUS "Either not Unix or Gtk frontend is disabled.")
  return()
endif()

set(GTK_SOURCES
  actions.cpp
  common.cpp
  dialog-eclipse.cpp
  dialog-goto.cpp
  dialog-options.cpp
  dialog-solar.cpp
  dialog-star.cpp
  dialog-time.cpp
  dialog-tour.cpp
  glwidget.cpp
  main.cpp
  menu-context.cpp
  settings-file.cpp
  splash.cpp
)

set(GTK_HEADERS
  actions.h
  common.h
  dialog-eclipse.h
  dialog-goto.h
  dialog-options.h
  dialog-solar.h
  dialog-star.h
  dialog-time.h
  dialog-tour.h
  glwidget.h
  menu-context.h
  settings-file.h
  splash.h
  ui.h
)

if (NOT USE_GTKGLEXT)
  set(GTK_SOURCES ${GTK_SOURCES} gtkegl.c)
  set(GTK_HEADERS ${GTK_HEADERS} gtkegl.h)
endif()

if (USE_GTK3)
  pkg_check_modules(GTK gtk+-3.0 REQUIRED)
else()
  pkg_check_modules(GTK gtk+-2.0 REQUIRED)
endif()
pkg_check_modules(CAIRO cairo)

if (ENABLE_GLES)
  pkg_check_modules(GLESv2 glesv2 REQUIRED)
endif()

if (USE_GTKGLEXT)
  pkg_check_modules(GTK2GLEXT gtkglext-1.0 REQUIRED)
else()
  pkg_check_modules(EGL egl REQUIRED)
endif()

add_executable(celestia-gtk ${GTK_SOURCES})
add_dependencies(celestia-gtk celestia)

if(${CMAKE_VERSION} VERSION_LESS "3.13.0")
  function(target_link_directories target scope)
    link_directories(${ARGN})
  endfunction()
endif()

target_include_directories(celestia-gtk PRIVATE ${GTK_INCLUDE_DIRS})
target_link_directories(celestia-gtk PRIVATE ${GTK_LIBRARY_DIRS})
target_link_libraries(celestia-gtk celestia ${GTK_LIBRARIES})
target_compile_options(celestia-gtk PRIVATE "-Wno-deprecated-declarations") # this option is directory-wide

if (CAIRO_FOUND)
  target_include_directories(celestia-gtk PRIVATE ${CAIRO_INCLUDE_DIRS})
  target_link_directories(celestia-gtk PRIVATE ${CAIRO_LIBRARY_DIRS})
  target_link_libraries(celestia-gtk ${CAIRO_LIBRARIES})
  target_compile_definitions(celestia-gtk PRIVATE CAIRO)
endif()

if (USE_GTKGLEXT)
  target_include_directories(celestia-gtk PRIVATE ${GTK2GLEXT_INCLUDE_DIRS})
  target_link_directories(celestia-gtk PRIVATE ${GTK2GLEXT_LIBRARY_DIRS})
  target_link_libraries(celestia-gtk celestia ${GTK2GLEXT_LIBRARIES})
  target_compile_definitions(celestia-gtk PRIVATE GTKGLEXT)
else()
  target_include_directories(celestia-gtk PRIVATE ${EGL_INCLUDE_DIRS})
  target_link_directories(celestia-gtk PRIVATE ${EGL_LIBRARY_DIRS})
  target_link_libraries(celestia-gtk celestia ${EGL_LIBRARIES})
endif()

if (ENABLE_GLES)
  target_include_directories(celestia-gtk PRIVATE ${GLESv2_INCLUDE_DIRS})
  target_link_directories(celestia-gtk PRIVATE ${GLESv2_LIBRARY_DIRS})
  target_link_libraries(celestia-gtk celestia ${GLESv2_LIBRARIES})
endif()

install(TARGETS celestia-gtk RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

add_subdirectory(data)
