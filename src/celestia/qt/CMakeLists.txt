if(NOT ENABLE_QT)
  message(STATUS "Qt frontend is disabled.")
  return()
endif()

if(APPLE AND EXISTS /usr/local/opt/qt5)
  # Homebrew installs Qt5 (up to at least 5.9.1) in
  # /usr/local/qt5, ensure it can be found by CMake since
  # it is not in the default /usr/local prefix.
  list(APPEND CMAKE_PREFIX_PATH "/usr/local/opt/qt5")
endif()

if(USE_QT6)
  find_package(Qt6 COMPONENTS Widgets OpenGLWidgets Core5Compat CONFIG REQUIRED)
else()
  find_package(Qt5 COMPONENTS Widgets CONFIG REQUIRED)
endif()

set(QT_SOURCES
  qtappwin.cpp
  qtbookmark.cpp
  qtcelestialbrowser.cpp
  qtcelestiaactions.cpp
  qtcolorswatchwidget.cpp
  qtdeepskybrowser.cpp
  qteventfinder.cpp
  qtglwidget.cpp
  qtgotoobjectdialog.cpp
  qtinfopanel.cpp
  qtmain.cpp
  qtpreferencesdialog.cpp
  qtselectionpopup.cpp
  qtsettimedialog.cpp
  qtsolarsystembrowser.cpp
  qttimetoolbar.cpp
  xbel.cpp
)

set(QT_HEADERS
  qtappwin.h
  qtbookmark.h
  qtcelestialbrowser.h
  qtcelestiaactions.h
  qtcolorswatchwidget.h
  qtdeepskybrowser.h
  qteventfinder.h
  qtgettext.h
  qtgotoobjectdialog.h
  qtglwidget.h
  qtinfopanel.h
  qtpreferencesdialog.h
  qtselectionpopup.h
  qtsettimedialog.h
  qtsolarsystembrowser.h
  qttimetoolbar.h
  xbel.h
)

# Instruct CMake to run moc automatically when needed
set(CMAKE_AUTOMOC ON)
# Create code from a list of Qt designer ui files
set(CMAKE_AUTOUIC ON)
# Find includes in corresponding build directories
set(CMAKE_INCLUDE_CURRENT_DIR ON)
if(USE_QT6)
  qt6_add_resources(RC_SRC "icons.qrc")
else()
  qt5_add_resources(RC_SRC "icons.qrc")
endif()

if(WIN32)
  set (RES celestia.rc)
endif()

add_executable(celestia-qt WIN32 ${QT_SOURCES} ${RC_SRC} ${RES})
add_dependencies(celestia-qt celestia)
if(USE_QT6)
  target_link_libraries(celestia-qt Qt6::Widgets Qt6::OpenGLWidgets Qt6::Core5Compat celestia)
else()
  target_link_libraries(celestia-qt Qt5::Widgets celestia)
endif()
if(APPLE)
  set_property(TARGET celestia-qt APPEND_STRING PROPERTY LINK_FLAGS " -framework CoreFoundation")
  set_property(TARGET celestia-qt APPEND_STRING PROPERTY LINK_FLAGS " -framework CoreServices")
endif()
install(TARGETS celestia-qt RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

add_subdirectory(data)
