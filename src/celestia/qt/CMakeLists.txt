if(NOT ENABLE_QT6)
  message(STATUS "Qt6 frontend is disabled.")
  return()
endif()

set(QT_NO_CREATE_VERSIONLESS_FUNCTIONS ON)
set(QT_NO_CREATE_VERSIONLESS_TARGETS ON)

set(CELQT6_SOURCES
  qtappwin.cpp
  qtappwin.h
  qtbookmark.cpp
  qtbookmark.h
  qtcelestiaactions.cpp
  qtcelestiaactions.h
  qtcelestialbrowser.cpp
  qtcelestialbrowser.h
  qtcolorswatchwidget.cpp
  qtcolorswatchwidget.h
  qtcommandline.cpp
  qtcommandline.h
  qtdateutil.cpp
  qtdateutil.h
  qtdeepskybrowser.cpp
  qtdeepskybrowser.h
  qtdraghandler.cpp
  qtdraghandler.h
  qteventfinder.cpp
  qteventfinder.h
  qtgettext.h
  qtglwidget.cpp
  qtglwidget.h
  qtgotoobjectdialog.cpp
  qtgotoobjectdialog.h
  qtinfopanel.cpp
  qtinfopanel.h
  qtmain.cpp
  qtmarkerutil.cpp
  qtmarkerutil.h
  qtpathutil.h
  qtpreferencesdialog.cpp
  qtpreferencesdialog.h
  qtselectionpopup.cpp
  qtselectionpopup.h
  qtsettimedialog.cpp
  qtsettimedialog.h
  qtsolarsystembrowser.cpp
  qtsolarsystembrowser.h
  qttimetoolbar.cpp
  qttimetoolbar.h
  qttourguide.cpp
  qttourguide.h
  xbel.cpp
  xbel.h
)

if(WIN32)
  list(APPEND CELQT6_SOURCES celestia.rc)
endif()

# Instruct CMake to run moc automatically when needed
set(CMAKE_AUTOMOC ON)
# Create code from a list of Qt designer ui files
set(CMAKE_AUTOUIC ON)

find_package(Qt6 COMPONENTS Core Widgets OpenGLWidgets CONFIG REQUIRED)
qt6_add_resources(CELQT6_RC_SRC "../qt/icons.qrc")

add_executable(celestia-qt6 WIN32 ${CELQT6_SOURCES} ${CELQT6_RC_SRC})
add_dependencies(celestia-qt6 celestia)
target_link_libraries(celestia-qt6 PUBLIC Qt6::Widgets Qt6::OpenGLWidgets celestia)

set_target_properties(celestia-qt6 PROPERTIES CXX_VISIBILITY_PRESET hidden)

if(APPLE)
  set_property(TARGET celestia-qt6 APPEND_STRING PROPERTY LINK_FLAGS " -framework CoreFoundation")
  set_property(TARGET celestia-qt6 APPEND_STRING PROPERTY LINK_FLAGS " -framework CoreServices")
endif()

if(WIN32)
  install(
    TARGETS celestia-qt6
    RUNTIME_DEPENDENCIES
      PRE_EXCLUDE_REGEXES "^api-ms" "^ext-ms-"
      POST_EXCLUDE_REGEXES ".*system32/.*\\.dll$"
      DIRECTORIES $<TARGET_FILE_DIR:celestia> $<TARGET_FILE_DIR:Qt6::Core>
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    COMPONENT qt6gui
  )
else()
  install(
    TARGETS celestia-qt6
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    COMPONENT qt6gui
  )
endif()

if(ENABLE_LTO)
  set_target_properties(celestia-qt6 PROPERTIES INTERPROCEDURAL_OPTIMIZATION ON)
endif()

if(WIN32)
  qt6_generate_deploy_app_script(TARGET celestia-qt6
    OUTPUT_SCRIPT deploy-celestia-qt6
    NO_UNSUPPORTED_PLATFORM_ERROR
    NO_COMPILER_RUNTIME
    NO_TRANSLATIONS
  )
  install(
    SCRIPT ${deploy-celestia-qt6}
    COMPONENT qt6gui
  )
endif()

add_subdirectory(data)
