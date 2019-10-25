set(CELX_SOURCES
  celx_category.cpp
  celx_category.h
  celx_celestia.cpp
  celx_celestia.h
  celx.cpp
  celx_frame.cpp
  celx_frame.h
  celx_gl.cpp
  celx_gl.h
  celx.h
  celx_internal.h
  celx_misc.cpp
  celx_misc.h
  celx_object.cpp
  celx_object.h
  celx_observer.cpp
  celx_observer.h
  celx_phase.cpp
  celx_phase.h
  celx_position.cpp
  celx_position.h
  celx_rotation.cpp
  celx_rotation.h
  celx_vector.cpp
  celx_vector.h
  luaplugin.cpp
  luascript.cpp
  luascript.h
  scriptobject.cpp
  scriptobject.h
  scriptorbit.cpp
  scriptorbit.h
  scriptrotation.cpp
  scriptrotation.h
)

add_library(celluascript MODULE ${CELX_SOURCES})

if(ENABLE_PLUGINS)
  target_compile_options(celluascript PUBLIC -fvisibility=hidden PUBLIC -fvisibility-inlines-hidden)
endif()
