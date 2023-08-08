if(NOT ENABLE_QT5)
  message(STATUS "Qt5 frontend is disabled, not building cmodview-qt5.")
  return()
endif()

include(../cmodview/CmodviewCommon.cmake)

if(APPLE AND EXISTS /usr/local/opt/qt5)
  # Homebrew installs Qt5 (up to at least 5.9.1) in
  # /usr/local/qt5, ensure it can be found by CMake since
  # it is not in the default /usr/local prefix.
  list(APPEND CMAKE_PREFIX_PATH "/usr/local/opt/qt5")
endif()

find_package(Qt5 COMPONENTS Widgets OpenGL CONFIG REQUIRED)

GetCmodviewSources()

# Instruct CMake to run moc automatically when needed
set(CMAKE_AUTOMOC ON)

build_cmod_tool(cmodview-qt5 WIN32 ${CMODVIEW_SOURCES})
target_include_directories(cmodview-qt5 PRIVATE ${OPENGL_INCLUDE_DIRS})
target_compile_definitions(cmodview-qt5 PRIVATE IMPORT_GLSUPPORT)
target_link_libraries(cmodview-qt5 Qt5::Widgets ${OPENGL_LIBRARIES})

if(APPLE)
  # Qt5 and Qt6 installed via homebrew conflict, so re-add the core include directories
  target_include_directories(cmodview-qt5 PRIVATE ${Qt5Core_INCLUDE_DIRS})
endif()

if(NOT MSVC)
  target_compile_options(cmodview-qt5 PRIVATE "-frtti")
endif()
