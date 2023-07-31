# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

find_path(
  MINIAUDIO_INCLUDE_DIR
  NAMES "miniaudio.h"
  PATH_SUFFIXES "include" "include/miniaudio"
  DOC "miniaudio include directory"
)
mark_as_advanced(MINIAUDIO_INCLUDE_DIR)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(
  Miniaudio
  FOUND_VAR MINIAUDIO_FOUND
  REQUIRED_VARS MINIAUDIO_INCLUDE_DIR
  FAIL_MESSAGE "Failed to find miniaudio")

if(MINIAUDIO_FOUND)
  set(MINIAUDIO_INCLUDE_DIRS "${MINIAUDIO_INCLUDE_DIR}")
  if(NOT TARGET miniaudio::miniaudio)
    add_library(miniaudio::miniaudio INTERFACE IMPORTED)
    set_target_properties(
      miniaudio::miniaudio PROPERTIES
      INTERFACE_INCLUDE_DIRECTORIES "${MINIAUDIO_INCLUDE_DIR}"
    )

    if(UNIX AND (NOT APPLE))
      set(THREADS_PREFER_PTHREAD_FLAG ON)
      find_package(Threads REQUIRED)
      target_link_libraries(miniaudio::miniaudio INTERFACE Threads::Threads)
    endif()
  endif()
endif()
