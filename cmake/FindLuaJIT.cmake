# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

#.rst:
# FindLuaJIT
# ---------
#
#
#
# Locate Lua library This module defines
#
# ::
#
#   LUAJIT_FOUND, if false, do not try to link to Lua
#   LUA_LIBRARIES
#   LUA_INCLUDE_DIR, where to find lua.h
#   LUA_VERSION_STRING, the version of Lua found (since CMake 2.8.8)
#   LUA_VERSION_MAJOR  - the major version of Lua
#   LUA_VERSION_MINOR  - the minor version of Lua
#   LUA_VERSION_PATCH  - the patch version of Lua
#   LUAJIT_VERSION_STRING, the version of LuaJIT found (since CMake 2.8.8)
#   LUAJIT_VERSION_MAJOR  - the major version of LuaJIT
#   LUAJIT_VERSION_MINOR  - the minor version of LuaJIT
#   LUAJIT_VERSION_PATCH  - the patch version of LuaJIT
#
#
#
# Note that the expected include convention is
#
# ::
#
#   #include "lua.h"
#
# and not
#
# ::
#
#   #include <lua/lua.h>
#
# This is because, the lua location is not standardized and may exist in
# locations other than lua/

find_path(LUA_INCLUDE_DIR luajit.h
  HINTS
    ENV LUA_DIR
  PATH_SUFFIXES include/luajit-2.0 include/luajit-2.1 include/luajit
  PATHS
  ~/Library/Frameworks
  /Library/Frameworks
  /sw # Fink
  /opt/local # DarwinPorts
  /opt/csw # Blastwave
  /opt
)

find_library(LUA_LIBRARY
  NAMES luajit-5.1 lua51
  HINTS
    ENV LUA_DIR
  PATH_SUFFIXES lib
  PATHS
  ~/Library/Frameworks
  /Library/Frameworks
  /sw
  /opt/local
  /opt/csw
  /opt
)

if(LUA_LIBRARY)
  # include the math library for Unix
  if(UNIX AND NOT APPLE AND NOT BEOS AND NOT HAIKU)
    find_library(LUA_MATH_LIBRARY m)
    set( LUA_LIBRARIES "${LUA_LIBRARY};${LUA_MATH_LIBRARY};${CMAKE_DL_LIBS}" CACHE STRING "Lua Libraries")
  # For Windows and Mac, don't need to explicitly include the math library
  else()
    set( LUA_LIBRARIES "${LUA_LIBRARY}" CACHE STRING "Lua Libraries")
  endif()
endif()

if(LUA_INCLUDE_DIR AND EXISTS "${LUA_INCLUDE_DIR}/lua.h")
  file(STRINGS "${LUA_INCLUDE_DIR}/lua.h" lua_version_str REGEX "^#define[ \t]+LUA_RELEASE[ \t]+\"Lua .+\"")

  string(REGEX REPLACE "^#define[ \t]+LUA_RELEASE[ \t]+\"Lua ([^\"]+)\".*" "\\1" LUA_VERSION_STRING "${lua_version_str}")
  unset(lua_version_str)
  string(REGEX REPLACE "^([0-9]+)\\.[0-9.]*$" "\\1" LUA_VERSION_MAJOR "${LUA_VERSION_STRING}")
  string(REGEX REPLACE "^[0-9]+\\.([0-9]+)[0-9.]*$" "\\1" LUA_VERSION_MINOR "${LUA_VERSION_STRING}")
  string(REGEX REPLACE "^[0-9]+\\.[0-9]+\\.([0-9]).*" "\\1" LUA_VERSION_PATCH "${LUA_VERSION_STRING}")
endif()

if(LUA_INCLUDE_DIR AND EXISTS "${LUA_INCLUDE_DIR}/luajit.h")
  file(STRINGS "${LUA_INCLUDE_DIR}/luajit.h" lua_version_str REGEX "^#define[ \t]+LUAJIT_VERSION[ \t]+\".+\"")
  string(REGEX REPLACE "^#define[ \t]+LUAJIT_VERSION[ \t]+\"LuaJIT ([0-9.]+).*\"" "\\1" LUAJIT_VERSION_STRING "${lua_version_str}")
  unset(lua_version_str)
  string(REGEX REPLACE "^([0-9]+)\\.[0-9.]*$" "\\1"  LUAJIT_VERSION_MAJOR "${LUAJIT_VERSION_STRING}")
  string(REGEX REPLACE "^[0-9]+\\.([0-9]+)[0-9.]*$" "\\1" LUAJIT_VERSION_MINOR "${LUAJIT_VERSION_STRING}")
  string(REGEX REPLACE "^[0-9]+\\.[0-9]+\\.([0-9]).*" "\\1" LUAJIT_VERSION_PATCH "${LUAJIT_VERSION_STRING}")
endif()

include(FindPackageHandleStandardArgs)
# handle the QUIETLY and REQUIRED arguments and set LUA_FOUND to TRUE if
# all listed variables are TRUE
find_package_handle_standard_args(LuaJIT
                                  REQUIRED_VARS LUA_LIBRARIES LUA_INCLUDE_DIR
                                  VERSION_VAR LUAJIT_VERSION_STRING)

mark_as_advanced(LUA_INCLUDE_DIR LUA_LIBRARIES LUA_LIBRARY LUA_MATH_LIBRARY)
