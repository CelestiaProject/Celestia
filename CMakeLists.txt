cmake_minimum_required(VERSION 3.1.0)

option(LEGACY_OPENGL_LIBS "Use legacy OpenGL libraries instead of glvnd library (Default: off)" OFF)

# Honor link flags in try_compile()
if (POLICY CMP0056)
  cmake_policy(SET CMP0056 NEW)
endif()

# only use the additional link flags when linking executables if the
# ENABLE_EXPORTS target property is set to True
if(POLICY CMP0065)
  cmake_policy(SET CMP0065 NEW)
endif()

# Honor language standard (CMAKE_CXX_STANDARD) in try_compile()
if(POLICY CMP0067)
  cmake_policy(SET CMP0067 NEW)
endif()

# Ignore generated source files in AUTOMOC and AUTOUIC
if(POLICY CMP0071)
  cmake_policy(SET CMP0071 OLD)
endif()

# Prefer GLVND or "legacy" OpenGL library (libOpenGL.so vs libGL.so)
if(POLICY CMP0072)
  if(LEGACY_OPENGL_LIBS)
    cmake_policy(SET CMP0072 OLD)
  else()
    cmake_policy(SET CMP0072 NEW)
  endif()
endif()

# Remove leading and trailing whitespace from libraries linked
if(POLICY CMP0004)
  cmake_policy(SET CMP0004 OLD)
endif()

project(celestia VERSION 1.7.0 LANGUAGES C CXX)
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
option(ENABLE_SDL     "Build SDL frontend? (Default: off)" OFF)
option(ENABLE_WIN     "Build Windows native frontend? (Default: on)" ON)
option(ENABLE_THEORA  "Support video capture to OGG Theora? (Default: on)" ON)
option(ENABLE_TOOLS   "Build different tools? (Default: off)" OFF)
option(NATIVE_OSX_APP "Support native OSX paths read data from (Default: off)" OFF)
option(FAST_MATH      "Build with unsafe fast-math compiller option (Default: off)" OFF)
option(ENABLE_TESTS   "Enable unit tests? (Default: off)" OFF)
option(ENABLE_DATA    "Install data from content submodule? (Default: on)" ON)
option(ENABLE_GLES    "Build for OpenGL ES 2.0 instead of OpenGL 2.1 (Default: off)" OFF)

if(ENABLE_GLES)
  add_definitions(-DGL_ES)
endif()

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Build type." FORCE)
endif()

set(CMAKE_INCLUDE_CURRENT_DIR ON)

if(UNIX AND (NOT APPLE) AND (NOT CYGWIN))
  set(_UNIX true)
endif()

# _USE_MATH_DEFINES enables use of math constants like M_PI,
# which are by default disabled in standard C++ mode (like std=c++11 instead of std=gnu11)
add_definitions(-D_USE_MATH_DEFINES)

# Let CMake handle setting C++11 (since 3.1)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_C_STANDARD 11)

# Use our cmake files
list(INSERT CMAKE_MODULE_PATH 0 "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

if((NOT WIN32) AND (NOT NATIVE_OSX_APP))
  include(GNUInstallDirs)
  include(rpath)
endif()
include(CheckIncludeFiles)
include(CheckIncludeFileCXX)
include(FastMath)
if(NOT WIN32)
  include(cotire)
else()
  macro(cotire _target)
  endmacro()
endif()

#
# Compile options
#
if(MSVC)
  # Disabled warnings
  # C4244, C4267, C4503: implicit type conversion to a smaller type
  # C4786: long names in templates
  # C4800: forcing value to bool
  # C4455: reserved literals (no undescore at the beginning)
  # C5030: not recognized attribute
  # Additional options
  # bigobj: generate more object sections than allowed by default
  # Promoted to errors
  # C4316: object allocated on the heap may not be aligned 16
  add_compile_options("/wd4244" "/wd4267" "/wd4503" "/wd4800" "/wd4455" "/wd5030" "/bigobj" "/we4316")
endif()

if(NOT WIN32)
  # Qt requires -fPIC, so build all code with it
  # It's always set on windows
  add_compile_options(-fPIC)
endif()

if(MINGW)
  # MinGW has a bug which causes too many false-positive warnings
  # for 'operator!='
  add_compile_options(-Wno-attributes)
endif()

EnableFastMath(${FAST_MATH})

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

  # begin static libintl support
  # libintl.a may depend on libiconv.a and libcharset.a
  # this is the case of vcpkg + mingw, for example
  macro(try_compile_intl)
    set(_argv ${ARGV})
    try_compile(INTLLIB ${CMAKE_BINARY_DIR}
                "${CMAKE_SOURCE_DIR}/checks/intl.cpp"
                LINK_LIBRARIES ${_argv}
                CMAKE_FLAGS "-DINCLUDE_DIRECTORIES=${Intl_INCLUDE_DIRS}")
  endmacro()

  try_compile_intl(${Intl_LIBRARIES})
  if (NOT INTLLIB)
    message(STATUS "Checking for libiconv and libcharset dependencies of libintl")
    find_package(Iconv REQUIRED)
    link_libraries(${Iconv_LIBRARIES})

    try_compile_intl(${Intl_LIBRARIES} ${Iconv_LIBRARIES})
    if (NOT INTLLIB)
      find_library(Charset_LIBRARY NAMES libcharset charset HINTS Charset_DIR)
      try_compile_intl(${Intl_LIBRARIES} ${Iconv_LIBRARIES} ${Charset_LIBRARY})
      if (NOT INTLLIB)
        message(FATAL_ERROR "LibIntl has unknown dependencies")
      else()
        link_libraries(${Charset_LIBRARY})
      endif()
    endif()
  endif()
  # end static libintl support

  add_definitions(-DENABLE_NLS)
else()
  message(STATUS "NLS is disabled. Not looking for gettext and libintl.")
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
  else()
    include_directories(${CSPICE_INCLUDE_DIR})
  endif()
  add_definitions(-DUSE_SPICE)
else()
  message(STATUS "NAIF SPICE is disabled. Not looking for cspice library.")
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

if(WIN32)
  link_libraries("vfw32" "comctl32" "winmm")
endif()

if(NOT ENABLE_GLES)
  find_package(OpenGL REQUIRED)
  include_directories(${OPENGL_INCLUDE_DIRS})
  link_libraries(${OPENGL_LIBRARIES})
endif()

find_package(Libepoxy REQUIRED)
link_libraries(libepoxy::libepoxy)
include_directories(${LIBEPOXY_INCLUDE_DIR})

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
else()
  message(STATUS "CELX is disabled. Not looking for LUA libraries.")
endif()

find_package(Freetype REQUIRED)
link_libraries(Freetype::Freetype)

#[[
get_cmake_property(_variableNames VARIABLES)
list (SORT _variableNames)
foreach (_variableName ${_variableNames})
    message(STATUS "${_variableName}=${${_variableName}}")
endforeach()
]]#

# macOS Bundling
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
elseif(WIN32)
  set(CMAKE_INSTALL_BINDIR ".")
  set(CMAKE_INSTALL_DATAROOTDIR ".")
  set(CMAKE_INSTALL_DATADIR ".")
  set(CMAKE_INSTALL_FULL_LOCALEDIR "locale")
  set(DATADIR "${CMAKE_INSTALL_DATADIR}")
  set(FULL_DATADIR ".")
else()
  set(DATADIR "${CMAKE_INSTALL_DATADIR}/${PROJECT_NAME}")
  set(FULL_DATADIR "${CMAKE_INSTALL_FULL_DATADIR}/${PROJECT_NAME}")
endif()

if(NOT GIT_COMMIT)
  find_program(GIT_FOUND git)
  if(GIT_FOUND)
    execute_process(
      COMMAND git log --pretty=format:%h -1
      WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
      OUTPUT_VARIABLE GIT_COMMIT
    )
    if("${GIT_COMMIT}" STREQUAL "")
      set(GIT_COMMIT "unknown")
    endif()
  else()
    set(GIT_COMMIT "unknown")
  endif()
endif()

#
# Compile definitions
#
add_definitions(
  -DVERSION="${PROJECT_VERSION}"
  -DPACKAGE="celestia"
  -DGIT_COMMIT="${GIT_COMMIT}"
)

if(NOT NATIVE_OSX_APP)
  add_definitions(
    -DLOCALEDIR="${CMAKE_INSTALL_FULL_LOCALEDIR}"
    -DCONFIG_DATA_DIR="${FULL_DATADIR}"
    -DHIP_DATA_DIR="${FULL_DATADIR}"
    -DSPLASH_DIR="${FULL_DATADIR}/splash"
  )
endif()

if(WIN32)
  add_definitions(
    -D_CRT_SECURE_NO_WARNINGS
    -D_SCL_SECURE_NO_WARNINGS
    -D_CRT_NON_CONFORMING_SWPRINTFS
    -DNOMINMAX
    -DWIN32_LEAN_AND_MEAN
    -DWINVER=0x0601
    -D_WIN32_WINNT=0x0601
  )
  # Fix the issue: https://github.com/CelestiaProject/Celestia/issues/364
  add_definitions(-D_ENABLE_EXTENDED_ALIGNED_STORAGE)
endif()

if(APPLE)
  if(NATIVE_OSX_APP)
    add_definitions(-DNATIVE_OSX_APP)
  endif()
  add_definitions(-DGL_SILENCE_DEPRECATION)
endif()

string(TOLOWER "${CMAKE_BUILD_TYPE}" build_type_lc)
if("${build_type_lc}" STREQUAL "debug")
  add_definitions(-D_DEBUG -DDEBUG)
else()
  add_definitions(-DNO_DEBUG -DEIGEN_NO_DEBUG)
  if(NOT MSVC)
    add_compile_options(-fno-rtti -fno-exceptions)
  endif()
endif()

# Turning all debug on dramatically decreases performance
if(OCTREE_DEBUG)
  add_definitions(-DOCTREE_DEBUG)
endif()

include_directories("${CMAKE_SOURCE_DIR}/src" ${CMAKE_BINARY_DIR})

# configure a header file to pass some of the CMake settings
# to the source code
include(CheckSymbolExists)
check_symbol_exists(wordexp wordexp.h HAVE_WORDEXP)
check_include_files(byteswap.h HAVE_BYTESWAP_H)
try_compile(HAVE_FILESYSTEM ${CMAKE_BINARY_DIR} "${CMAKE_SOURCE_DIR}/checks/cxxfs.cpp")
check_include_file_cxx(experimental/filesystem HAVE_EXPERIMENTAL_FILESYSTEM)

if((NOT HAVE_FILESYSTEM) AND HAVE_EXPERIMENTAL_FILESYSTEM)
  # Try compile without linking against any library
  try_compile(CXXFSLIB ${CMAKE_BINARY_DIR} "${CMAKE_SOURCE_DIR}/checks/cxxfsexp.cpp")

  if(CXXFSLIB)
    # Nothing to do, linking is not explicitly required
  else()
    # Find the filesystem lib we need to link against
    if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
      find_library(CLANG_FS_LIBRARY "c++fs")
      if(CLANG_FS_LIBRARY)
        set(FS_LIBRARY c++fs)
      else()
        set(FS_LIBRARY stdc++fs)
      endif()
    elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
      set(FS_LIBRARY stdc++fs)
    endif()

    try_compile(CXXFSLIB ${CMAKE_BINARY_DIR} "${CMAKE_SOURCE_DIR}/checks/cxxfs.cpp" LINK_LIBRARIES "${FS_LIBRARY}")
    if(CXXFSLIB)
      link_libraries("${FS_LIBRARY}")
    else()
      set(HAVE_EXPERIMENTAL_FILESYSTEM OFF)
      message(WARNING "C++ experimental filesystem library is unusable!\nWill use own implementation.")
    endif()
  endif()
endif()

try_compile(HAVE_STRING_VIEW ${CMAKE_BINARY_DIR} "${CMAKE_SOURCE_DIR}/checks/cxxsv.cpp")
try_compile(HAVE_EXPERIMENTAL_STRING_VIEW ${CMAKE_BINARY_DIR} "${CMAKE_SOURCE_DIR}/checks/cxxsvexp.cpp")

configure_file("config.h.in" "config.h")

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
add_subdirectory(fonts)
add_subdirectory(images)
add_subdirectory(locale)
add_subdirectory(scripts)
add_subdirectory(shaders)

if(ENABLE_DATA)
  add_subdirectory(content)
endif()

if(ENABLE_TESTS)
  enable_testing()
  add_subdirectory(test)
endif()
