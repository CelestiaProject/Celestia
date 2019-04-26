cmake_minimum_required(VERSION 3.1.0)

option(LEGACY_OPENGL_LIBS "Use legacy OpenGL libraries instead of glvnd library (Default: off)" OFF)

if (POLICY CMP0071)
  cmake_policy(SET CMP0071 OLD)
endif()

if (POLICY CMP0072)
  if (LEGACY_OPENGL_LIBS)
    cmake_policy(SET CMP0072 OLD)
  else()
    cmake_policy(SET CMP0072 NEW)
  endif()
endif()

project(celestia)

set(VERSION "1.7.0")
set(DISPLAY_NAME "Celestia")
#
#
#
option(ENABLE_CELX    "Enable celx scripting, requires Lua library? (Default: on)" ON)
option(ENABLE_SPICE   "Use spice library? (Default: off)" OFF)
option(ENABLE_NLS     "Enable interface translation? (Default: on)" ON)
option(ENABLE_GLUT    "Build simple Glut frontend? (Default: on)" OFF)
option(ENABLE_GTK     "Build GTK2 frontend (Unix only)? (Default: off)" OFF)
option(ENABLE_QT      "Build Qt frontend? (Default: on)" ON)
option(ENABLE_WIN     "Build Windows native frontend? (Default: on)" ON)
option(ENABLE_THEORA  "Support video capture to OGG Theora? (Default: on)" ON)
option(ENABLE_TOOLS   "Build different tools? (Default: off)" OFF)
option(NATIVE_OSX_APP "Support native OSX paths read data from (Default: off)" OFF)
option(FAST_MATH      "Build with unsafe fast-math compiller option (Default: off)" OFF)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Build type." FORCE)
endif()

set(CMAKE_INCLUDE_CURRENT_DIR ON)

if(NOT WIN32)
  # Qt requires -fPIC, so build all code with it
  # It's always set on windows
  add_compile_options(-fPIC)
endif()

if(FAST_MATH)
  if(NOT MSVC)
    add_compile_options("-ffast-math")
  else()
    add_compile_options("/fp:fast")
  endif()
endif()

if(MSVC)
  # Disabled warnings
  # C4244, C4267, C4503: implicit type conversion to a smaller type
  # C4786: long names in templates
  # C4800: forcing value to bool
  # Additional options
  # bigobj: generate more object sections than allowed by default
  add_compile_options("/wd4244" "/wd4267" "/wd4503" "/wd4800" "/bigobj")

  # Fix the issue: https://github.com/CelestiaProject/Celestia/issues/364
  add_definitions(-D_ENABLE_EXTENDED_ALIGNED_STORAGE)
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
include(CheckIncludeFileCXX)
include(rpath)
include(install_to_extras_subdir)
if (NOT MSVC AND NOT MINGW)
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

  find_package(Intl REQUIRED)
  include_directories(${Intl_INCLUDE_DIRS})
  link_libraries(${Intl_LIBRARIES})
endif()

if(ENABLE_SPICE)
  find_package(Cspice)
  if(NOT CSPICE_FOUND)
    message(STATUS "Using cspice submodule")
    add_subdirectory("${CMAKE_SOURCE_DIR}/thirdparty/Spice")
    get_target_property(SPICE_INCLUDE_DIR Spice INCLUDE_DIRECTORIES)
    include_directories(${SPICE_INCLUDE_DIR})
    message(STATUS "Spice include directories: ${SPICE_INCLUDE_DIR}")
    add_library(CSPICE::CSPICE ALIAS Spice)
  endif()
  add_definitions(-DUSE_SPICE)
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

find_package(GLEW REQUIRED)
link_libraries(GLEW::GLEW)
include_directories(${GLEW_INCLUDE_DIRS})

find_package(Eigen3 3.3 NO_MODULE) # -DEigen3_DIR=...
if(TARGET Eigen3::Eigen)
  message(STATUS "Found Eigen3 ${EIGEN3_VERSION_STRING}")
  link_libraries(Eigen3::Eigen)
else()
  message(STATUS "Using Eigen submodule")
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

find_package(PNG REQUIRED)
add_definitions(${PNG_DEFINITIONS})
link_libraries(PNG::PNG)

find_package(JPEG REQUIRED) # -DJPEG_LIBRARY=...
include_directories(${JPEG_INCLUDE_DIRS})
link_libraries(${JPEG_LIBRARIES})

if(ENABLE_CELX)
  add_definitions(-DCELX)

  find_package(LuaJIT)
  if(NOT LUAJIT_FOUND)
    find_package(Lua REQUIRED)
  endif()
  include_directories(${LUA_INCLUDE_DIR})
  link_libraries(${LUA_LIBRARIES})
endif()

#[[
get_cmake_property(_variableNames VARIABLES)
list (SORT _variableNames)
foreach (_variableName ${_variableNames})
    message(STATUS "${_variableName}=${${_variableName}}")
endforeach()
]]#

########### macOS Bundling ###############
if(NATIVE_OSX_APP)
  message(STATUS "Generating Info.plist for macOS Bundle")
  configure_file(${CMAKE_SOURCE_DIR}/Info.plist.in ${CMAKE_BINARY_DIR}/Info.plist @ONLY)
endif()

if(NATIVE_OSX_APP)
  set(CMAKE_INSTALL_PREFIX "${PROJECT_BINARY_DIR}/${PROJECT_NAME}.app/Contents")
  set(CMAKE_INSTALL_FULL_LOCALEDIR "${CMAKE_INSTALL_PREFIX}/Resources/locale")
  set(CMAKE_INSTALL_BINDIR "MacOS")
  set(DATADIR "${CMAKE_INSTALL_PREFIX}/Resources")
  set(FULL_DATADIR "${CMAKE_INSTALL_PREFIX}/Resources")
else()
  set(DATADIR "${CMAKE_INSTALL_DATADIR}/${PROJECT_NAME}")
  set(FULL_DATADIR "${CMAKE_INSTALL_FULL_DATADIR}/${PROJECT_NAME}")
endif()

if (NOT GIT_COMMIT)
  find_program(GIT_FOUND git)
  if (GIT_FOUND)
    if (NOT MINGW)
      execute_process(
        COMMAND git log --pretty=format:"%h" -1
        WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
        OUTPUT_VARIABLE GIT_COMMIT
      )
    endif()
    if (MINGW)
      execute_process(
        COMMAND git log --pretty=format:'%h' -1
        WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
        OUTPUT_VARIABLE GIT_COMMIT
      )
    endif()
    if ("${GIT_COMMIT}" STREQUAL "")
      set(GIT_COMMIT "unknown")
    endif()
  else()
    set(GIT_COMMIT "unknown")
  endif()
  
  if (MINGW)
    set(GIT_COMMIT "\"${GIT_COMMIT}\"")
  endif()
endif()

add_definitions(
  -DVERSION="${VERSION}"
  -DPACKAGE="celestia"
  -DGIT_COMMIT=${GIT_COMMIT}
)

if(NOT NATIVE_OSX_APP)
  add_definitions(
    -DLOCALEDIR="${CMAKE_INSTALL_FULL_LOCALEDIR}"
    -DCONFIG_DATA_DIR="${FULL_DATADIR}"
    -DHIP_DATA_DIR="${FULL_DATADIR}"
    -DSPLASH_DIR="${FULL_DATADIR}/splash/"
  )
endif()

if(WIN32)
  add_definitions(
    -D_CRT_SECURE_NO_WARNINGS
    -D_SCL_SECURE_NO_WARNINGS
    -DNOMINMAX
    -DWIN32_LEAN_AND_MEAN
    -DWINVER=0x0601
    -D_WIN32_WINNT=0x0601
  )
endif()

if(APPLE)
  # TARGET_OS_MAC should be replaced with __APPLE__
  add_definitions(-DTARGET_OS_MAC)
  # Dirty hack to prevent AIFF.h inclusion which provides `struct Marker`
  # conflicting with out `class Marker`.
  add_definitions(-D__AIFF__)
  if (NATIVE_OSX_APP)
    add_definitions(-DNATIVE_OSX_APP)
  endif()
endif()

string(TOLOWER "${CMAKE_BUILD_TYPE}" build_type_lc)
if(NOT "${build_type_lc}" STREQUAL "debug")
  add_definitions(-DNO_DEBUG -DEIGEN_NO_DEBUG)
endif()

# Turning all debug on dramatically decreses performance
if(OCTREE_DEBUG)
  add_definitions(-DOCTREE_DEBUG)
endif()

include_directories("${CMAKE_SOURCE_DIR}/src" ${CMAKE_BINARY_DIR})

# configure a header file to pass some of the CMake settings
# to the source code
check_include_files(byteswap.h HAVE_BYTESWAP_H)
check_include_file_cxx(experimental/filesystem HAVE_EXPERIMENTAL_FILESYSTEM)
configure_file("config.h.in" "config.h")


if (HAVE_EXPERIMENTAL_FILESYSTEM)
  if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    find_library(CLANG_FS_LIBRARY "c++fs")
    if (CLANG_FS_LIBRARY)
      set(FS_LIBRARY c++fs)
    else()
      set(FS_LIBRARY stdc++fs)
    endif()
  elseif (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    set(FS_LIBRARY stdc++fs)
  endif()
  link_libraries("${FS_LIBRARY}")
endif()

set(BASE_DATA_SOURCES
  demo.cel
  guide.cel
  start.cel
  celestia.cfg
  controls.txt
)
install(FILES ${BASE_DATA_SOURCES} DESTINATION ${DATADIR})
install(FILES "splash.png" DESTINATION "${DATADIR}/splash")

if(NATIVE_OSX_APP)
  install(FILES ${CMAKE_BINARY_DIR}/Info.plist DESTINATION .)
endif()

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
add_subdirectory(shaders)
add_subdirectory(textures)
add_subdirectory(test)
