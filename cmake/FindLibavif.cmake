# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

#.rst:
# FindLibavif
# --------
#
# Find the avif headers and libraries.
#
# This module reports information about the avif
# installation in several variables.  General variables::
#
#   LIBAVIF_FOUND - true if the avif headers and libraries were found
#   LIBAVIF_INCLUDE_DIRS - the directory containing the avif headers
#   LIBAVIF_LIBRARIES - avif libraries to be linked
#
# The following cache variables may also be set::
#
#   LIBAVIF_INCLUDE_DIR - the directory containing the avif headers
#   LIBAVIF_LIBRARY - the avif library (if any)

# Find include directory

# TODO: use pkgconfig

find_path(LIBAVIF_INCLUDE_DIR
          NAMES avif/avif.h
          HINTS LIBAVIF_DIR
          DOC "avif headers")
mark_as_advanced(LIBAVIF_INCLUDE_DIR)

find_library(LIBAVIF_LIBRARY
             NAMES avif
             HINTS LIBAVIF_DIR
             DOC "avif libraries")
mark_as_advanced(LIBAVIF_LIBRARY)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Libavif
                                  FOUND_VAR LIBAVIF_FOUND
                                  REQUIRED_VARS LIBAVIF_INCLUDE_DIR LIBAVIF_LIBRARY
                                  FAIL_MESSAGE "Failed to find avif")

if(LIBAVIF_FOUND)
  set(LIBAVIF_INCLUDE_DIRS "${LIBAVIF_INCLUDE_DIR}")
  if(LIBAVIF_LIBRARY)
    set(LIBAVIF_LIBRARIES "${LIBAVIF_LIBRARY}")
  else()
    unset(LIBAVIF_LIBRARIES)
  endif()

  if(NOT TARGET libavif::libavif)
    add_library(libavif::libavif UNKNOWN IMPORTED)
    set_target_properties(libavif::libavif PROPERTIES
      INTERFACE_INCLUDE_DIRECTORIES "${LIBAVIF_INCLUDE_DIRS}")
    set_target_properties(libavif::libavif PROPERTIES
      IMPORTED_LOCATION "${LIBAVIF_LIBRARY}")
  endif()
endif()
