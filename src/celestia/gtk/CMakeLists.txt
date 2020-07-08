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

pkg_check_modules(GTK2 gtk+-2.0 REQUIRED)
pkg_check_modules(GTK2GLEXT gtkglext-1.0 REQUIRED)

add_executable(celestia-gtk ${GTK_SOURCES})
add_dependencies(celestia-gtk celestia)
cotire(celestia-gtk)
target_include_directories(celestia-gtk PRIVATE ${GTK2_INCLUDE_DIRS} ${GTK2GLEXT_INCLUDE_DIRS})
target_link_libraries(celestia-gtk celestia ${GTK2_LIBRARIES} ${GTK2GLEXT_LIBRARIES})
install(TARGETS celestia-gtk RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

add_subdirectory(data)
