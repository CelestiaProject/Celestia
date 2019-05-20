if(NOT ENABLE_QT)
  message("Qt frontend is disabled, not building qttxf.")
  return()
endif()

if(APPLE AND EXISTS /usr/local/opt/qt5)
  # Homebrew installs Qt5 (up to at least 5.9.1) in
  # /usr/local/qt5, ensure it can be found by CMake since
  # it is not in the default /usr/local prefix.
  list(APPEND CMAKE_PREFIX_PATH "/usr/local/opt/qt5")
endif()

set(QT_LIBS Widgets)
find_package(Qt5 COMPONENTS ${QT_LIBS} CONFIG REQUIRED)

# Instruct CMake to run moc automatically when needed
set(CMAKE_AUTOMOC ON)
# Find includes in corresponding build directories
set(CMAKE_INCLUDE_CURRENT_DIR ON)

add_executable(qttxf WIN32 main.cpp  mainwindow.cpp)
install(TARGETS qttxf RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
qt5_use_modules(qttxf ${QT_LIBS})
