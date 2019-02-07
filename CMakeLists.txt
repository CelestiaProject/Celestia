cmake_minimum_required(VERSION 3.1.0)

set(LEGACY_OPENGL_LIBS OFF CACHE BOOL "Use legacy OpenGL libraries instead of glvnd library (Default: off)")

if (POLICY CMP0072)
  if (LEGACY_OPENGL_LIBS)
    cmake_policy(SET CMP0072 OLD)
  else()
    cmake_policy(SET CMP0072 NEW)
  endif()
endif()

project(celestia)

set(VERSION "1.7.0")
#
#
#
set(ENABLE_CELX     ON  CACHE BOOL "Enable celx scripting, requires Lua library? (Default: on)")
set(ENABLE_SPICE    OFF CACHE BOOL "Use spice library? (Default: off)")
set(ENABLE_NLS      ON  CACHE BOOL "Enable interface translation? (Default: on)")
set(ENABLE_GLUT     ON  CACHE BOOL "Build simple Glut frontend? (Default: on)")
set(ENABLE_GTK      OFF CACHE BOOL "Build GTK2 frontend (Unix only)? (Default: off)")
set(ENABLE_QT       ON  CACHE BOOL "Build Qt frontend? (Default: on)")
set(ENABLE_WIN      ON  CACHE BOOL "Build Windows native frontend? (Default: on)")
set(ENABLE_THEORA   ON  CACHE BOOL "Support video capture to OGG Theora? (Default: on)")

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Build type." FORCE)
endif()

set(CMAKE_INCLUDE_CURRENT_DIR ON)

if(NOT MSVC)
  # Qt requires -fPIC, so build all code with it
  add_compile_options(-fPIC)
else()
  # Disabled warnings
  # C4244: implicit type conversion to a smaller type
  # C4503: type truncation
  # C4786: long names in templates
  # C4800: forcing value to bool
  add_compile_options("/wd4244" "/wd4503" "/wd4786" "/wd4800")
endif()

if(UNIX AND (NOT APPLE) AND (NOT CYGWIN))
  set(_UNIX true)
endif()

# Theora supported on Unix-like systems only, but not on OSX
if(NOT _UNIX)
  set(ENABLE_THEORA OFF)
endif()

# _USE_MATH_DEFINES enables use of math constants like M_PI,
# which are by default disabled in standard C++ mode (like std=c++11 instead of std=gnu11)
add_definitions(-D_USE_MATH_DEFINES)

# Let CMake handle setting C++11 (since 3.1)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_C_STANDARD 11)

# Use our cmake files
list(INSERT CMAKE_MODULE_PATH 0 "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

include(GNUInstallDirs)
include(CheckIncludeFiles)
include(rpath)
include(install_to_extras_subdir)
if (NOT MSVC)
  include(cotire)
else()
  macro(cotire _target)
  endmacro()
endif()

#
# NLS (Gettext) support
#
if(ENABLE_NLS)
  find_package(Gettext)
  if(NOT GETTEXT_FOUND)
    message(WARNING "Gettext is not found, traslations won't be created.")
  endif()
  include(FixGettext)

  include(FindIntl)
  find_package(Intl REQUIRED)
  include_directories(${Intl_INCLUDE_DIRS})
  link_libraries(${Intl_LIBRARIES})
endif()

if(ENABLE_SPICE)
  include(FindCspice)
  find_package(Cspice)
  if(NOT CSPICE_FOUND)
    message(STATUS "Using cspice submodule")
    add_subdirectory("${CMAKE_SOURCE_DIR}/thirdparty/Spice")

    include_directories("${CMAKE_SOURCE_DIR}/thirdparty/Spice/include")
    add_library(CSPICE::CSPICE ALIAS Spice)
  endif()
endif()

if(_UNIX)
  find_package(PkgConfig)
endif()

if(_UNIX AND ENABLE_THEORA)
  pkg_search_module(THEORA theora REQUIRED)
  include_directories(${THEORA_INCLUDE_DIRS})
  link_libraries(${THEORA_LIBRARIES})
  add_definitions(-DTHEORA)
endif()

if (WIN32)
  link_libraries("vfw32" "comctl32" "winmm")
endif()

find_package(OpenGL REQUIRED)
include_directories(${OPENGL_INCLUDE_DIRS})
link_libraries(${OPENGL_LIBRARIES})

find_package(GLEW)
if(GLEW_FOUND)
  link_libraries(GLEW::GLEW)
  include_directories(${GLEW_INCLUDE_DIRS})
elseif(_UNIX)
  pkg_search_module(GLEW glew)
  if(GLEW_FOUND)
    include_directories(${GLEW_INCLUDE_DIRS})
    link_libraries(${GLEW_LIBRARIES})
  else()
    message(FATAL_ERROR "glew was not found")
  endif()
else()
  message(FATAL_ERROR "glew was not found")
endif()

find_package(Eigen3 3.3 NO_MODULE) # -DEigen3_DIR=...
if(TARGET Eigen3::Eigen)
  message(STATUS "Found Eigen3 ${EIGEN3_VERSION_STRING}")
  link_libraries(Eigen3::Eigen)
elseif(_UNIX)
  pkg_search_module(EIGEN eigen3)
  if(EIGEN_FOUND)
    include_directories(${EIGEN_INCLUDE_DIRS})
  else()
    message(STATUS "Using Eigen submodule")
    include_directories("${CMAKE_SOURCE_DIR}/thirdparty/Eigen")
  endif()
else()
  include_directories("${CMAKE_SOURCE_DIR}/thirdparty/Eigen")
endif()

find_package(fmt 4.0.0 CONFIG QUIET)
if(fmt_FOUND)
  link_libraries(fmt::fmt)
else()
  message(STATUS "Using fmt submodule")
  add_subdirectory("${CMAKE_SOURCE_DIR}/thirdparty/fmt")
  link_libraries(fmt::fmt)
endif()

find_package(PNG)
if(PNG_FOUND)
  add_definitions(${PNG_DEFINITIONS})
  include_directories(${PNG_INCLUDE_DIRS})
  link_libraries(${PNG_LIBRARIES})
elseif(_UNIX)
  pkg_search_module(PNG libpng REQUIRED)
  include_directories(${PNG_INCLUDE_DIRS})
  link_libraries(${PNG_LIBRARIES})
else()
  message(FATAL_ERROR "libpng was not found")
endif()

find_package(JPEG) # -DJPEG_LIBRARY=...
if(JPEG_FOUND)
  include_directories(${JPEG_INCLUDE_DIRS})
  link_libraries(${JPEG_LIBRARIES})
elseif(_UNIX)
  pkg_search_module(JPEG libjpeg REQUIRED)
  include_directories(${JPEG_INCLUDE_DIRS})
  link_libraries(${JPEG_LIBRARIES})
else()
  message(FATAL_ERROR "libjpeg was not found")
endif()

if(ENABLE_CELX)
  add_definitions(-DCELX)

  find_package(Lua)
  if(LUA_FOUND)
    add_definitions(-DLUA_VER=0x0${LUA_VERSION_MAJOR}0${LUA_VERSION_MINOR}00)
    include_directories(${LUA_INCLUDE_DIR})
    link_libraries(${LUA_LIBRARIES})
  else()
    if(_UNIX)
      pkg_search_module(LUA luajit lua5.3 lua5.2 lua5.1 REQUIRED)
      include_directories(${LUA_INCLUDE_DIRS})
      link_libraries(${LUA_LIBRARIES})

      if(LUA_VERSION VERSION_EQUAL "5.3" OR LUA_VERSION VERSION_GREATER "5.3")
        add_definitions(-DLUA_VER=0x050300)
      elseif(LUA_VERSION VERSION_EQUAL "5.2" OR LUA_VERSION VERSION_GREATER "5.2")
        add_definitions(-DLUA_VER=0x050200)
      elseif(LUA_VERSION VERSION_EQUAL "5.1" OR LUA_VERSION VERSION_GREATER "5.1")
        add_definitions(-DLUA_VER=0x050100)
      else() # luajit
        add_definitions(-DLUA_VER=0x050100)
      endif()
    elseif(WIN32 OR APPLE)
      add_definitions(-DLUA_VER=0x050100)
    endif()
  endif()
endif()

#[[
get_cmake_property(_variableNames VARIABLES)
list (SORT _variableNames)
foreach (_variableName ${_variableNames})
    message(STATUS "${_variableName}=${${_variableName}}")
endforeach()
]]#

if(ENABLE_SPICE)
  add_definitions(-DUSE_SPICE)
endif()

if(WIN32)
  add_definitions(-D_CRT_SECURE_NO_WARNINGS -D_SCL_SECURE_NO_WARNINGS)
  add_definitions(-DNOMINMAX -DWIN32_LEAN_AND_MEAN)
  add_definitions(-DWINVER=0x0601 -D_WIN32_WINNT=0x0601)
endif()

set(DATADIR "${CMAKE_INSTALL_DATADIR}/${PROJECT_NAME}")
set(FULL_DATADIR "${CMAKE_INSTALL_FULL_DATADIR}/${PROJECT_NAME}")

if (NOT GIT_COMMIT)
  find_program(GIT_FOUND git)
  if (GIT_FOUND)
    execute_process(
      COMMAND git log --pretty=format:"%h" -1
      WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
      OUTPUT_VARIABLE GIT_COMMIT
    )
  else()
    set(GIT_COMMIT "unknown")
  endif()
endif()

add_definitions(
  -DVERSION="${VERSION}"
  -DLOCALEDIR="${CMAKE_INSTALL_FULL_LOCALEDIR}"
  -DPACKAGE="celestia"
  -DCONFIG_DATA_DIR="${FULL_DATADIR}"
  -DHIP_DATA_DIR="${FULL_DATADIR}"
  -DSPLASH_DIR="${FULL_DATADIR}/splash/"
  -DGIT_COMMIT=${GIT_COMMIT}
)

if(APPLE)
  # TARGET_OS_MAC should be replaced with __APPLE__
  add_definitions(-DTARGET_OS_MAC)
  # Dirty hack to prevent AIFF.h inclusion which provides `struct Marker`
  # conflicting with out `class Marker`.
  add_definitions(-D__AIFF__)
endif()

string(TOLOWER "${CMAKE_BUILD_TYPE}" build_type_lc)
if(NOT "${build_type_lc}" STREQUAL "debug")
  add_definitions(-DNO_DEBUG -DEIGEN_NO_DEBUG)
endif()

include_directories("${CMAKE_SOURCE_DIR}/src" ${CMAKE_BINARY_DIR})

# configure a header file to pass some of the CMake settings
# to the source code
configure_file("config.h.in" "config.h")
check_include_files(byteswap.h HAVE_BYTESWAP_H)

set(BASE_DATA_SOURCES
  demo.cel
  guide.cel
  start.cel
  celestia.cfg
  controls.txt
)
install(FILES ${BASE_DATA_SOURCES} DESTINATION ${DATADIR})
install(FILES "splash.png" DESTINATION "${DATADIR}/splash")

add_subdirectory(src)
add_subdirectory(po)
add_subdirectory(po2)
add_subdirectory(data)
add_subdirectory(extras)
add_subdirectory(extras-standard)
add_subdirectory(fonts)
add_subdirectory(images)
add_subdirectory(locale)
add_subdirectory(models)
add_subdirectory(scripts)
add_subdirectory(textures)
