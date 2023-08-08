if(NOT ENABLE_QT6)
  message(STATUS "Qt6 frontend is disabled, not building cmodview-qt6.")
  return()
endif()

include(../cmodview/CmodviewCommon.cmake)

find_package(Qt6 COMPONENTS Core CONFIG REQUIRED)
# Work around a bug in the Core5Compat cmake files which reference the
# non-versioned Qt::Core and Qt::CorePrivate libraries
add_library(Qt::Core ALIAS Qt6::Core)
add_library(Qt::CorePrivate ALIAS Qt6::CorePrivate)
find_package(Qt6 COMPONENTS Core Widgets OpenGLWidgets Core5Compat CONFIG REQUIRED)

GetCmodviewSources()

# Instruct CMake to run moc automatically when needed
set(CMAKE_AUTOMOC ON)

build_cmod_tool(cmodview-qt6 WIN32 ${CMODVIEW_SOURCES})
target_include_directories(cmodview-qt6 PRIVATE ${OPENGL_INCLUDE_DIRS})
target_compile_definitions(cmodview-qt6 PRIVATE IMPORT_GLSUPPORT)
target_link_libraries(cmodview-qt6 Qt6::Widgets Qt6::OpenGLWidgets Qt6::Core5Compat ${OPENGL_LIBRARIES})

if(APPLE)
  set_property(TARGET cmodview-qt6 APPEND_STRING PROPERTY LINK_FLAGS " -framework CoreFoundation")
  set_property(TARGET cmodview-qt6 APPEND_STRING PROPERTY LINK_FLAGS " -framework CoreServices")
endif()

if(NOT MSVC)
  target_compile_options(cmodview-qt6 PRIVATE "-frtti")
endif()

if(WIN32)
  qt6_generate_deploy_app_script(TARGET cmodview-qt6
    OUTPUT_SCRIPT deploy-cmodview-qt6
    NO_UNSUPPORTED_PLATFORM_ERROR
    NO_TRANSLATIONS
  )
  install(
    SCRIPT "${deploy-cmodview-qt6}"
    COMPONENT tools
  )
endif()
