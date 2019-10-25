# These object files are merged in the celegine library
set(CELPLUGIN_SOURCES
  plugin-common.h
  plugin.cpp
  plugin.h
  pluginmanager.cpp
  pluginmanager.h
)
add_library(celplugin OBJECT ${CELPLUGIN_SOURCES})

