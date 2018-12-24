macro(find_ffmpeg_lib)
  if(NOT(${ARGC} EQUAL 3))
    message(FATAL_ERROR "find_ffmpeg_lib requires exactly 3 arguments")
  endif()

  set(__name   ${ARGV0})
  set(__header ${ARGV1})
  set(__lib    ${ARGV2})

  find_library(${__name}_LIBRARY ${__lib})
  find_path(${__name}_INCLUDE_DIR ${__header})
  find_package_handle_standard_args(${__name}
                                  FOUND_VAR ${__name}_FOUND
                                  REQUIRED_VARS ${__name}_INCLUDE_DIR ${__name}_LIBRARY
                                  FAIL_MESSAGE "Failed to find ${__name}")

  set(${__name}_INCLUDE_DIRS ${${__name}_INCLUDE_DIR})
  set(${__name}_LIBRARIES ${${__name}_LIBRARY})

  list(APPEND FFMPEG_INCLUDE_DIRS ${${__name}_INCLUDE_DIR})
  list(REMOVE_DUPLICATES FFMPEG_INCLUDE_DIRS)
  list(APPEND FFMPEG_LIBRARIES ${${__name}_LIBRARY})
  list(REMOVE_DUPLICATES FFMPEG_LIBRARIES)

  if(NOT TARGET FFMPEG::${__name})
    add_library(FFMPEG::${__name} UNKNOWN IMPORTED)
    set_target_properties(FFMPEG::${__name} PROPERTIES
      INTERFACE_INCLUDE_DIRECTORIES "${${__name}_INCLUDE_DIR}"
      IMPORTED_LINK_INTERFACE_LANGUAGES "C"
      IMPORTED_LOCATION "${${__name}_LIBRARY}")
  endif()

  mark_as_advanced(${__name}_INCLUDE_DIR ${__name}_LIBRARY ${__name}_INCLUDE_DIRS ${__name}_LIBRARIES)
endmacro()

include(FindPackageHandleStandardArgs)
if(FFMPEG_FIND_COMPONENTS)
  foreach(component ${FFMPEG_FIND_COMPONENTS})
    string(TOUPPER ${component} _COMPONENT)
    set(FFMPEG_USE_${_COMPONENT} 1)
  endforeach()
endif()

set(FFMPEG_INCLUDE_DIRS)
set(FFMPEG_LIBRARIES)

if(FFMPEG_USE_AVCODEC)
  find_ffmpeg_lib(AVCODEC libavcodec/avcodec.h avcodec)
endif()
if(FFMPEG_USE_AVFORMAT)
  find_ffmpeg_lib(AVFORMAT libavformat/avformat.h avformat)
endif()
if(FFMPEG_USE_AVUTIL)
  find_ffmpeg_lib(AVUTIL libavutil/avutil.h avutil)
endif()
if(FFMPEG_USE_AVDEVICE)
  find_ffmpeg_lib(AVDEVICE libavdevice/avdevice.h avdevice)
endif()
if(FFMPEG_USE_SWSCALE)
  find_ffmpeg_lib(SWSCALE libswscale/swscale.h swscale)
endif()

if(NOT TARGET FFMPEG::FFMPEG)
  add_library(FFMPEG::FFMPEG UNKNOWN IMPORTED)
  set_target_properties(FFMPEG::FFMPEG PROPERTIES IMPORTED_LINK_INTERFACE_LANGUAGES "C")
  if(TARGET FFMPEG::AVCODEC)
    set_target_properties(FFMPEG::FFMPEG PROPERTIES INTERFACE_LINK_LIBRARIES FFMPEG::AVCODEC)
  endif()
  if(TARGET FFMPEG::AVUTIL)
    set_target_properties(FFMPEG::FFMPEG PROPERTIES INTERFACE_LINK_LIBRARIES FFMPEG::AVUTIL)
  endif()
  if(TARGET FFMPEG::AVUTIL)
    set_target_properties(FFMPEG::FFMPEG PROPERTIES INTERFACE_LINK_LIBRARIES FFMPEG::AVUTIL)
  endif()
  if(TARGET FFMPEG::AVFORMAT)
   set_target_properties(FFMPEG::FFMPEG PROPERTIES INTERFACE_LINK_LIBRARIES FFMPEG::AVFORMAT)
  endif()
  if(TARGET FFMPEG::SWSCALE)
    set_target_properties(FFMPEG::FFMPEG PROPERTIES INTERFACE_LINK_LIBRARIES FFMPEG::SWSCALE)
  endif()
endif()
