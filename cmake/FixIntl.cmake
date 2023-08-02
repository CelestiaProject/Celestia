# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

# CMake's FindIntl module only creates a target library since 3.20
# Since we support back to 3.8 (and Ubuntu 18.04 supplies 3.10),
# manually create the target if necessary

macro(MakeIntlTarget)
  if(NOT TARGET Intl::Intl)
    add_library(Intl::Intl INTERFACE IMPORTED)
    set_target_properties(
      Intl::Intl PROPERTIES
      INTERFACE_INCLUDE_DIRECTORIES "${Intl_INCLUDE_DIRS}"
      INTERFACE_LINK_LIBRARIES "${Intl_LIBRARIES}"
    )
  endif()
endmacro()
