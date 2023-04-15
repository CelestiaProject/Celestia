if(NOT ENABLE_QT5)
  message(STATUS "Qt5 frontend is disabled.")
  return()
endif()

include(../qt/CelestiaQtCommon.cmake)

if(APPLE AND EXISTS /usr/local/opt/qt5)
  # Homebrew installs Qt5 (up to at least 5.9.1) in
  # /usr/local/qt5, ensure it can be found by CMake since
  # it is not in the default /usr/local prefix.
  list(APPEND CMAKE_PREFIX_PATH "/usr/local/opt/qt5")
endif()

# Instruct CMake to run moc automatically when needed
set(CMAKE_AUTOMOC ON)
# Create code from a list of Qt designer ui files
set(CMAKE_AUTOUIC ON)

find_package(Qt5 COMPONENTS Widgets CONFIG REQUIRED)
qt5_add_resources(RC_SRC "../qt/icons.qrc")

if(USE_WAYLAND)
  GetQtSources(ON)
else()
  GetQtSources(OFF)
endif()

add_executable(celestia-qt5 WIN32 ${QT_SOURCES} ${RES} ${RC_SRC})
add_dependencies(celestia-qt5 celestia)
target_link_libraries(celestia-qt5 Qt5::Widgets celestia)

if(USE_WAYLAND)
  target_link_libraries(celestia-qt5 Wayland::Client wayland-protocols-helper)
  target_compile_definitions(celestia-qt5 PRIVATE USE_WAYLAND)
  target_include_directories(celestia-qt5 PRIVATE ${Qt5Gui_PRIVATE_INCLUDE_DIRS})
endif()

if(APPLE)
  # Qt5 and Qt6 installed via homebrew conflict, so re-add the core include directories
  target_include_directories(celestia-qt5 PRIVATE ${Qt5Core_INCLUDE_DIRS})
endif()

if(APPLE)
  set_property(TARGET celestia-qt5 APPEND_STRING PROPERTY LINK_FLAGS " -framework CoreFoundation")
  set_property(TARGET celestia-qt5 APPEND_STRING PROPERTY LINK_FLAGS " -framework CoreServices")
endif()

install(TARGETS celestia-qt5 RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

add_subdirectory(data)
