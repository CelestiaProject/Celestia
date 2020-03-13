# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

#.rst:
# FindLibepoxy
# --------
#
# Find the epoxy headers and libraries.
#
# This module reports information about the epoxy
# installation in several variables.  General variables::
#
#   LIBEPOXY_FOUND - true if the epoxy headers and libraries were found
#   LIBEPOXY_INCLUDE_DIRS - the directory containing the epoxy headers
#   LIBEPOXY_LIBRARIES - epoxy libraries to be linked
#
# The following cache variables may also be set::
#
#   LIBEPOXY_INCLUDE_DIR - the directory containing the epoxy headers
#   LIBEPOXY_LIBRARY - the epoxy library (if any)

# Find include directory

# TODO: use pkgconfig

find_path(LIBEPOXY_INCLUDE_DIR
          NAMES epoxy/gl.h
          HINTS LIBEPOXY_DIR
          DOC "epoxy headers")
mark_as_advanced(LIBEPOXY_INCLUDE_DIR)

find_library(LIBEPOXY_LIBRARY
             NAMES epoxy
             HINTS LIBEPOXY_DIR
             DOC "epoxy libraries")
mark_as_advanced(LIBEPOXY_LIBRARY)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Libepoxy
                                  FOUND_VAR LIBEPOXY_FOUND
                                  REQUIRED_VARS LIBEPOXY_INCLUDE_DIR LIBEPOXY_LIBRARY
                                  FAIL_MESSAGE "Failed to find epoxy")

if(LIBEPOXY_FOUND)
  set(LIBEPOXY_INCLUDE_DIRS "${LIBEPOXY_INCLUDE_DIR}")
  if(LIBEPOXY_LIBRARY)
    set(LIBEPOXY_LIBRARIES "${LIBEPOXY_LIBRARY}")
  else()
    unset(LIBEPOXY_LIBRARIES)
  endif()

  if(NOT TARGET libepoxy::libepoxy)
    add_library(libepoxy::libepoxy UNKNOWN IMPORTED)
    set_target_properties(libepoxy::libepoxy PROPERTIES
      INTERFACE_INCLUDE_DIRECTORIES "${LIBEPOXY_INCLUDE_DIRS}")
    set_target_properties(libepoxy::libepoxy PROPERTIES
      IMPORTED_LOCATION "${LIBEPOXY_LIBRARY}")
  endif()
endif()
