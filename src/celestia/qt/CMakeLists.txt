if(NOT ENABLE_QT)
  message("Qt frontend is disabled.")
  return()
endif()

set(QT_LIBS Widgets OpenGL)
find_package(Qt5 COMPONENTS ${QT_LIBS} CONFIG REQUIRED)

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
qt5_add_resources(RC_SRC "icons.qrc")
add_executable(celestia-qt WIN32 ${QT_SOURCES} ${RC_SRC})
cotire(celestia-qt)
qt5_use_modules(celestia-qt ${QT_LIBS})
target_link_libraries(celestia-qt ${CELESTIA_LIBS})
install(TARGETS celestia-qt RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
