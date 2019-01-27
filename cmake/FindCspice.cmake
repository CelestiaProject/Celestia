# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

#.rst:
# FindCspice
# --------
#
# Find the NAIF toolkit cspice headers and libraries.
#
# This module reports information about the Gettext cspice
# installation in several variables.  General variables::
#
#   CSPICE_FOUND - true if the cspice headers and libraries were found
#   CSPICE_INCLUDE_DIRS - the directory containing the cspice headers
#   CSPICE_LIBRARIES - cspice libraries to be linked
#
# The following cache variables may also be set::
#
#   CSPICE_INCLUDE_DIR - the directory containing the cspice headers
#   CSPICE_LIBRARY - the cspice library (if any)

# Find include directory
find_path(CSPICE_INCLUDE_DIR
          NAMES "SpiceOsc.h"
          DOC "cspice include directory")
mark_as_advanced(CSPICE_INCLUDE_DIR)

# Find CSPICE library
find_library(CSPICE_LIBRARY
             NAMES "cspice" "libcspice" "cspice.a"
             DOC "cspice libraries")
mark_as_advanced(CSPICE_LIBRARY)

FIND_PACKAGE_HANDLE_STANDARD_ARGS(CSPICE
                                  FOUND_VAR CSPICE_FOUND
                                  REQUIRED_VARS CSPICE_INCLUDE_DIR CSPICE_LIBRARY
                                  FAIL_MESSAGE "Failed to find cspice")

if(CSPICE_FOUND)
  set(CSPICE_INCLUDE_DIRS "${CSPICE_INCLUDE_DIR}")
  if(CSPICE_LIBRARY)
    set(CSPICE_LIBRARIES "${CSPICE_LIBRARY}")
  else()
    unset(CSPICE_LIBRARIES)
  endif()

  if(NOT TARGET CSPICE::CSPICE)
    add_library(CSPICE::CSPICE UNKNOWN IMPORTED)
    set_target_properties(CSPICE::CSPICE PROPERTIES
      INTERFACE_INCLUDE_DIRECTORIES "${CSPICE_INCLUDE_DIRS}")
    set_target_properties(CSPICE::CSPICE PROPERTIES
      IMPORTED_LOCATION "${CSPICE_LIBRARY}")
  endif()
endif()
