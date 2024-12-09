if(WIN32)
  # Require more recent CMake on Windows because DLLs are a pain
  cmake_minimum_required(VERSION 3.19)
else()
  cmake_minimum_required(VERSION 3.9)
endif()

# Plain and keyword target_link_libraries() signatures cannot be mixed
if (POLICY CMP0023)
  cmake_policy(SET CMP0023 NEW)
endif()

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

# Remove leading and trailing whitespace from libraries linked
if(POLICY CMP0004)
  cmake_policy(SET CMP0004 NEW)
endif()

# Use Boost-provided FindBoost module instead of CMake-provided one
if(POLICY CMP0167)
  cmake_policy(SET CMP0167 NEW)
endif()

# Normalize install(DESTINATION) paths
if(POLICY CMP0177)
  cmake_policy(SET CMP0177 NEW)
endif()

project(celestia VERSION 1.7.0 LANGUAGES C CXX)
set(DISPLAY_NAME "Celestia")
#
#
#
option(ENABLE_CELX        "Enable celx scripting, requires Lua library? (Default: on)" ON)
option(ENABLE_SPICE       "Use spice library? (Default: off)" OFF)
option(ENABLE_NLS         "Enable interface translation? (Default: on)" ON)
option(ENABLE_GTK         "Build GTK2 frontend (Unix only)? (Default: off)" OFF)
option(ENABLE_QT5         "Build Qt frontend? (Default: off)" OFF)
option(ENABLE_QT6         "Build Qt6 frontend (Default: off)" OFF)
option(ENABLE_SDL         "Build SDL frontend? (Default: off)" OFF)
option(ENABLE_WIN         "Build Windows native frontend? (Default: on)" ON)
option(ENABLE_FFMPEG      "Support video capture using FFMPEG (Default: off)" OFF)
option(ENABLE_MINIAUDIO   "Support audio playback using miniaudio (Default: off)" OFF)
option(ENABLE_TOOLS       "Build different tools? (Default: off)" OFF)
option(ENABLE_FAST_MATH   "Build with unsafe fast-math compiller option (Default: off)" OFF)
option(ENABLE_TESTS       "Enable unit tests? (Default: off)" OFF)
option(ENABLE_GLES        "Build for OpenGL ES 2.0 instead of OpenGL 2.1 (Default: off)" OFF)
option(ENABLE_LTO         "Enable link time optimizations (Default: off)" OFF)
option(USE_GTKGLEXT       "Use libgtkglext1 for GTK2 frontend (Default: on)" ON)
option(USE_GTK3           "Use Gtk3 in GTK2 frontend (Default: off)" OFF)
option(USE_WAYLAND        "Use Wayland in Qt frontend (Default: off)" OFF)
option(USE_GLSL_STRUCTS   "Use structs in GLSL (Default: off)" OFF)
option(USE_ICU            "Use ICU for UTF8 decoding for text rendering (Default: off)" OFF)
option(USE_WIN_ICU        "Use Windows SDK's ICU implementation (Default: off)" OFF)
option(USE_MESHOPTIMIZER  "Use meshoptimizer (Default: off)" OFF)
option(USE_WEFFCPP        "Use the -Weffc++ option when compiling with GCC (Default: off)" OFF)
option(LEGACY_OPENGL_LIBS "Use legacy OpenGL libraries instead of glvnd library (Default: off)" OFF)

# Prefer GLVND or "legacy" OpenGL library (libOpenGL.so vs libGL.so)
if(LEGACY_OPENGL_LIBS)
  set(OpenGL_GL_PREFERENCE LEGACY)
else()
  set(OpenGL_GL_PREFERENCE GLVND)
endif()

# Qt requires -fPIC, so build all code with it
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

if(APPLE AND ENABLE_QT5 AND ENABLE_QT6)
  # If someone figures out how to resolve the include path conflicts, this can be removed
  message(FATAL_ERROR "Simultaneous builds of Qt5 and Qt6 are not supported on Apple systems")
endif()

if(ENABLE_GLES)
  add_definitions(-DGL_ES)
  # Disable USE_GTKGLEXT if using OpenGL ES
  set(USE_GTKGLEXT OFF)
endif()

if(USE_GLSL_STRUCTS)
  # GLES SL on some old Android device or with ANGLE presents a link failure with empty error when using struct.
  # GLSL on old version of Mac OS X appears to have a bug that precludes us from using structs.
  add_definitions(-DUSE_GLSL_STRUCTS)
endif()

# Use our cmake files
list(INSERT CMAKE_MODULE_PATH 0 "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

# Require Windows 7
if(WIN32)
  set(WINVER 0x0601)
  set(NTDDI_VER 0x06010000)
endif()

if(USE_ICU)
  add_definitions(-DUSE_ICU)
  if(WIN32 AND USE_WIN_ICU)
    include(winicu)
    EnableWinICU()
  else()
    find_package(ICU REQUIRED COMPONENTS uc i18n)
    link_libraries(ICU::uc ICU::i18n)
  endif()
endif()

if(USE_GTK3)
  # Disable USE_GTKGLEXT if using Gtk+3
  set(USE_GTKGLEXT OFF)
endif()

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Build type." FORCE)
endif()

set(CMAKE_INCLUDE_CURRENT_DIR ON)

if(UNIX AND (NOT CYGWIN))
  set(_UNIX true)
endif()

# _USE_MATH_DEFINES enables use of math constants like M_PI,
# which are by default disabled in standard C++ mode (like std=c++11 instead of std=gnu11)
add_definitions(-D_USE_MATH_DEFINES)

# Let CMake handle setting C++11 (since 3.1)
if(NOT CMAKE_CXX_STANDARD)
  set(CMAKE_CXX_STANDARD 17)
endif()
set(CMAKE_C_STANDARD 11)

if(NOT WIN32)
  include(GNUInstallDirs)
  include(rpath)
endif()
include(CheckIncludeFiles)
include(CheckIncludeFileCXX)
include(FastMath)

#
# Compile options
#
if(MSVC)
  # CMake 3.14 and below set warning flags by default, remove them to prevent conflicts
  string(REGEX REPLACE "/W[3|4]" "" CMAKE_C_FLAGS "${CMAKE_C_FLAGS}")
  string(REGEX REPLACE "/W[3|4]" "" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")

  # Additional options
  # utf-8: set source and execution character set to UTF-8
  # bigobj: generate more object sections than allowed by default
  # Promoted to errors
  # C4316: object allocated on the heap may not be aligned 16
  add_compile_options("/utf-8" "/bigobj" "/we4316")

  if(MSVC_VERSION GREATER_EQUAL 1914)
    # /Zc:__cplusplus: correctly set __cplusplus macro value
    add_compile_options("/Zc:__cplusplus")
  endif()
endif()

if(MINGW)
  # MinGW has a bug which causes too many false-positive warnings
  # for 'operator!='
  add_compile_options(-Wno-attributes)
endif()

# G++ 10 introduced static analyzer
if(USE_WEFFCPP AND CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
  string(REPLACE "." ";" GXX_VERSION_LIST ${CMAKE_CXX_COMPILER_VERSION})
  list(GET GXX_VERSION_LIST 0 MAJOR)
  if(MAJOR GREATER 9)
    add_compile_options($<$<COMPILE_LANGUAGE:CXX>:-Weffc++>)
  endif()
endif()

EnableFastMath(${ENABLE_FAST_MATH})

#
# NLS (Gettext) support
#
if(ENABLE_NLS)
  find_package(Gettext)
  if(NOT GETTEXT_FOUND)
    message(WARNING "Gettext is not found, translations won't be created.")
  endif()

  find_package(Intl REQUIRED)
  include(FixIntl)
  MakeIntlTarget()
  link_libraries(Intl::Intl)
  add_definitions(-DENABLE_NLS)
else()
  message(STATUS "NLS is disabled. Not looking for gettext and libintl.")
endif()

if(ENABLE_SPICE)
  find_package(CSPICE)
  if(NOT CSPICE_FOUND)
    message(STATUS "Using cspice submodule")
    add_subdirectory("${CMAKE_SOURCE_DIR}/thirdparty/Spice")
    get_target_property(SPICE_INCLUDE_DIR spice INCLUDE_DIRECTORIES)
    include_directories(${SPICE_INCLUDE_DIR})
    message(STATUS "Spice include directories: ${SPICE_INCLUDE_DIR}")
    add_library(CSPICE::CSPICE ALIAS spice)
  else()
    include_directories(${CSPICE_INCLUDE_DIR})
  endif()
  add_definitions(-DUSE_SPICE)
else()
  message(STATUS "NAIF SPICE is disabled. Not looking for cspice library.")
endif()

if(ENABLE_FFMPEG)
  include(FindFFMPEG)
  find_package(FFMPEG REQUIRED COMPONENTS avcodec avutil avformat swscale)
  link_libraries(FFMPEG::AVCODEC FFMPEG::AVUTIL FFMPEG::AVFORMAT FFMPEG::SWSCALE)
  add_definitions(-DUSE_FFMPEG)
endif()

if(ENABLE_MINIAUDIO)
  add_library(miniaudio INTERFACE)
  target_include_directories(miniaudio INTERFACE "${CMAKE_SOURCE_DIR}/thirdparty/miniaudio")

  if(UNIX AND (NOT APPLE))
    set(THREADS_PREFER_PTHREAD_FLAG ON)
    find_package(Threads REQUIRED)
    target_link_libraries(miniaudio INTERFACE Threads::Threads)
  endif()

  link_libraries(miniaudio)
  add_definitions(-DUSE_MINIAUDIO)
endif()

if(ENABLE_LIBAVIF)
  find_package(Libavif REQUIRED)
  link_libraries(libavif::libavif)
  include_directories(${LIBAVIF_INCLUDE_DIR})
  add_definitions(-DUSE_LIBAVIF)
endif()

if(_UNIX)
  find_package(PkgConfig)
endif()

if(WIN32)
  link_libraries("vfw32" "comctl32" "winmm")
endif()

if(ENABLE_WIN OR ENABLE_TOOLS)
  find_package(OpenGL REQUIRED)
endif()

find_package(Libepoxy REQUIRED)
link_libraries(libepoxy::libepoxy)
include_directories(${LIBEPOXY_INCLUDE_DIR})

find_package(Eigen3 3.3 REQUIRED NO_MODULE) # -DEigen3_DIR=...
message(STATUS "Found Eigen3 ${EIGEN3_VERSION_STRING}")
link_libraries(Eigen3::Eigen)

find_package(fmt 8.0.0 CONFIG QUIET)
if(NOT fmt_FOUND)
  message(STATUS "Using fmt submodule")
  add_subdirectory("${CMAKE_SOURCE_DIR}/thirdparty/fmt")
endif()
link_libraries(fmt::fmt)

find_package(PNG REQUIRED)
add_definitions(${PNG_DEFINITIONS})
link_libraries(PNG::PNG)

include(FixJPEG)
find_package(JPEG REQUIRED) # -DJPEG_LIBRARY=...
MakeJPEGTarget()
link_libraries(JPEG::JPEG)

if(ENABLE_CELX)
  add_definitions(-DCELX)

  find_package(LuaJIT)
  if(LUAJIT_FOUND)
    link_libraries(LuaJIT::LuaJIT)
  else()
    find_package(Lua REQUIRED)
    include(FixLua)
    MakeLuaTarget()
    link_libraries(Lua::Lua)
  endif()
else()
  message(STATUS "CELX is disabled. Not looking for LUA libraries.")
endif()

find_package(Freetype REQUIRED)
link_libraries(Freetype::Freetype)

if(USE_MESHOPTIMIZER)
  find_package(meshoptimizer CONFIG REQUIRED)
  set(HAVE_MESHOPTIMIZER 1)
endif()

if(USE_WAYLAND)
  message(STATUS "Creating Wayland protocol helpers")

  find_package(Wayland COMPONENTS protocols scanner REQUIRED)

  include(WaylandAddProtocolClient)
  wayland_add_protocol_client(PROTOCOL_SOURCES
    "unstable/pointer-constraints/pointer-constraints-unstable-v1.xml")
  wayland_add_protocol_client(PROTOCOL_SOURCES
    "unstable/relative-pointer/relative-pointer-unstable-v1.xml")

  set_source_files_properties(${PROTOCOL_SOURCES} PROPERTIES GENERATED ON)
  add_library(wayland-protocols-helper STATIC ${PROTOCOL_SOURCES})
  target_include_directories(wayland-protocols-helper INTERFACE ${CMAKE_CURRENT_BINARY_DIR})
endif()

find_package(gperf REQUIRED)

find_package(Boost REQUIRED)
link_libraries(Boost::boost)
add_definitions(-DBOOST_NO_EXCEPTIONS)

#[[
get_cmake_property(_variableNames VARIABLES)
list (SORT _variableNames)
foreach (_variableName ${_variableNames})
    message(STATUS "${_variableName}=${${_variableName}}")
endforeach()
]]#

if(WIN32)
  set(CMAKE_INSTALL_BINDIR ".")
  set(CMAKE_INSTALL_DATAROOTDIR ".")
  set(CMAKE_INSTALL_DATADIR ".")
  set(CMAKE_INSTALL_FULL_LOCALEDIR "locale")
  set(CMAKE_INSTALL_LOCALEDIR "${CMAKE_INSTALL_FULL_LOCALEDIR}")
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
  -DGIT_COMMIT="${GIT_COMMIT}"
)

add_definitions(
  -DLOCALEDIR="${CMAKE_INSTALL_FULL_LOCALEDIR}"
  -DCONFIG_DATA_DIR="${FULL_DATADIR}"
  -DHIP_DATA_DIR="${FULL_DATADIR}"
)

if(WIN32)
  add_definitions(
    -D_CRT_SECURE_NO_WARNINGS
    -D_SCL_SECURE_NO_WARNINGS
    -DNOMINMAX
    -DWIN32_LEAN_AND_MEAN
    -DUNICODE
    -D_UNICODE
    -DWINVER=${WINVER}
    -D_WIN32_WINNT=${WINVER}
    -DNTDDI_VERSION=${NTDDI_VER}
  )
  # Fix the issue: https://github.com/CelestiaProject/Celestia/issues/364
  add_definitions(-D_ENABLE_EXTENDED_ALIGNED_STORAGE)
endif()

if(APPLE)
  add_definitions(-DGL_SILENCE_DEPRECATION)
endif()

string(TOLOWER "${CMAKE_BUILD_TYPE}" build_type_lc)
if("${build_type_lc}" STREQUAL "debug")
  add_definitions(-D_DEBUG -DDEBUG)
  if(NOT MSVC)
    add_compile_options("-fsanitize=address" "-fsanitize=undefined" "-fno-omit-frame-pointer")
    add_link_options("-fsanitize=address" "-fsanitize=undefined" "-fno-omit-frame-pointer")
  endif()
else()
  add_definitions(-DNO_DEBUG -DEIGEN_NO_DEBUG)
  if(NOT MSVC)
    add_compile_options(
      $<$<COMPILE_LANGUAGE:CXX>:-fno-exceptions>
    )
    if(FMT_VERSION LESS 90000)
      add_compile_options(
        $<$<COMPILE_LANGUAGE:CXX>:-fno-rtti>
      )
    endif()
  endif()
endif()

if(ENABLE_LTO)
  if(WIN32)
    message(WARNING "LTO is not supported yet when building for Windows")
    set(ENABLE_LTO OFF)
  else()
    include(CheckIPOSupported)
    check_ipo_supported(RESULT lto_supported)
    if(lto_supported)
      message(STATUS "LTO is supported")
    else()
      message(WARNING "LTO is not supported")
      set(ENABLE_LTO OFF)
    endif()
  endif()
endif()

include_directories("${CMAKE_SOURCE_DIR}/src" ${CMAKE_BINARY_DIR})

# configure a header file to pass some of the CMake settings
# to the source code
include(CheckSymbolExists)
check_symbol_exists(wordexp wordexp.h HAVE_WORDEXP)
check_include_files(byteswap.h HAVE_BYTESWAP_H)

try_compile(HAVE_CONSTEXPR_CMATH ${CMAKE_BINARY_DIR} "${CMAKE_SOURCE_DIR}/checks/constexprcmath.cpp")
if(HAVE_CONSTEXPR_CMATH)
  message(STATUS "Found constexpr functions in <cmath>")
else()
  message(STATUS "No constexpr functions in <cmath>")
endif()

try_compile(HAVE_SINCOS ${CMAKE_BINARY_DIR} "${CMAKE_SOURCE_DIR}/checks/sincos.cpp")
if(HAVE_SINCOS)
  message(STATUS "Found sincos in <cmath>")
else()
  try_compile(HAVE_APPLE_SINCOS ${CMAKE_BINARY_DIR} "${CMAKE_SOURCE_DIR}/checks/applesincos.cpp")
  if(HAVE_APPLE_SINCOS)
    message(STATUS "Found __sincos in <cmath>")
  else()
    message(STATUS "No sincos function found")
  endif()
endif()
# The libc++ headers in the latest stable NDK (r25) does not expose empty array methods as constexpr
# https://github.com/android/ndk/issues/1530
try_compile(HAVE_CONSTEXPR_EMPTY_ARRAY ${CMAKE_BINARY_DIR} "${CMAKE_SOURCE_DIR}/checks/constexpremptyarray.cpp")
if(HAVE_CONSTEXPR_EMPTY_ARRAY)
  message(STATUS "Found constexpr empty array")
else()
  message(STATUS "Empty array is not constexpr")
endif()

find_package(Filesystem REQUIRED COMPONENTS Final Experimental)
if(CXX_FILESYSTEM_IS_EXPERIMENTAL)
  if(ENABLE_QT6)
    message(FATAL_ERROR "Qt6 requires <filesystem> but only <experimental/filesystem> present. Please update your compiler.")
  else()
    message(WARNING "C++ lacks header <filesystem>, using <experimental/filesystem> instead.")
  endif()
else()
  set(HAVE_STD_FILESYSTEM ON)
endif()
link_libraries(std::filesystem)

try_compile(HAVE_CHARCONV ${CMAKE_BINARY_DIR} "${CMAKE_SOURCE_DIR}/checks/cxxccint.cpp")
if(HAVE_CHARCONV)
  try_compile(HAVE_FLOAT_CHARCONV ${CMAKE_BINARY_DIR} "${CMAKE_SOURCE_DIR}/checks/cxxccfloat.cpp")
  if(NOT HAVE_FLOAT_CHARCONV)
    message(WARNING "C++ charconv lacks floating point support!\nWill use own implementation.")
  endif()
else()
  message(WARNING "C++ charconv is unusable!\nWill use own implementation.")
endif()

configure_file("config.h.in" "config.h")
configure_file("celestia.cfg.in" "celestia.cfg")

set(BASE_DATA_SOURCES
  demo.cel
  guide.cel
  start.cel
  ${CMAKE_BINARY_DIR}/celestia.cfg
  controls.txt
  COPYING
)
install(
  FILES ${BASE_DATA_SOURCES}
  DESTINATION ${DATADIR}
  COMPONENT core
)
install(
  FILES "splash.png"
  DESTINATION "${DATADIR}/splash"
  COMPONENT core
)

if(NOT WIN32)
  install(
    FILES celestia-logo.png
    DESTINATION "${CMAKE_INSTALL_DATADIR}/pixmaps"
    RENAME celestia.png
    COMPONENT core
  )
endif()

add_subdirectory(po)
add_subdirectory(src)
add_subdirectory(fonts)
add_subdirectory(images)
add_subdirectory(locale)
add_subdirectory(scripts)
add_subdirectory(shaders)
add_subdirectory(help)

if(ENABLE_TESTS)
  include(CTest)
  add_subdirectory(test)
endif()
