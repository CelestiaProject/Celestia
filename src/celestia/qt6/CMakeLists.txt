if(NOT ENABLE_QT6)
  message(STATUS "Qt6 frontend is disabled.")
  return()
endif()

include(../qt/CelestiaQtCommon.cmake)

# Instruct CMake to run moc automatically when needed
set(CMAKE_AUTOMOC ON)
# Create code from a list of Qt designer ui files
set(CMAKE_AUTOUIC ON)

# Work around a bug in the Core5Compat cmake files which reference the
# non-versioned Qt::Core and Qt::CorePrivate libraries
find_package(Qt6 COMPONENTS Core CONFIG REQUIRED)
add_library(Qt::Core ALIAS Qt6::Core)
add_library(Qt::CorePrivate ALIAS Qt6::CorePrivate)
find_package(Qt6 COMPONENTS Core Widgets OpenGLWidgets Core5Compat CONFIG REQUIRED)
qt6_add_resources(RC_SRC "../qt/icons.qrc")

if(USE_WAYLAND)
  GetQtSources(ON)
else()
  GetQtSources(OFF)
endif()

add_executable(celestia-qt6 WIN32 ${QT_SOURCES} ${RES} ${RC_SRC})
add_dependencies(celestia-qt6 celestia)
target_link_libraries(celestia-qt6 Qt6::Widgets Qt6::OpenGLWidgets Qt6::Core5Compat celestia)

if(USE_WAYLAND)
  target_link_libraries(celestia-qt6 Wayland::Client wayland-protocols-helper)
  target_compile_definitions(celestia-qt6 PRIVATE USE_WAYLAND)
  target_include_directories(celestia-qt6 PRIVATE ${Qt6Gui_PRIVATE_INCLUDE_DIRS})
endif()

if(APPLE)
  set_property(TARGET celestia-qt6 APPEND_STRING PROPERTY LINK_FLAGS " -framework CoreFoundation")
  set_property(TARGET celestia-qt6 APPEND_STRING PROPERTY LINK_FLAGS " -framework CoreServices")
endif()

install(TARGETS celestia-qt6 RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

if(WIN32)
  qt6_generate_deploy_app_script(TARGET celestia-qt6
    OUTPUT_SCRIPT deploy-celestia-qt6
    NO_UNSUPPORTED_PLATFORM_ERROR
    NO_TRANSLATIONS
  )
  install(SCRIPT "${deploy-celestia-qt6}")
endif()

add_subdirectory(data)
