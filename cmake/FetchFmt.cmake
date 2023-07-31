# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

if(NOT TARGET fmt::fmt)
  message(STATUS "Using fmt external project download")
  include(ExternalProject)

  ExternalProject_Add(
    fmt_ext
    URL "https://github.com/fmtlib/fmt/archive/refs/tags/6.1.0.tar.gz"
    URL_HASH SHA256=8FB84291A7ED6B4DB4769115B57FA56D5467B1AB8C3BA5BDF78C820E4BD17944
    BUILD_BYPRODUCTS
      "<INSTALL_DIR>/lib/${CMAKE_STATIC_LIBRARY_PREFIX}fmt${CMAKE_STATIC_LIBRARY_SUFFIX}"
      "<INSTALL_DIR>/lib/${CMAKE_STATIC_LIBRARY_PREFIX}fmtd${CMAKE_STATIC_LIBRARY_SUFFIX}"
    CMAKE_ARGS
      -DCMAKE_INSTALL_PREFIX:PATH=<INSTALL_DIR>
      -DCMAKE_INSTALL_LIBDIR=lib
      -DCMAKE_POSITION_INDEPENDENT_CODE=ON
      -DFMT_TEST=OFF
      -DBUILD_SHARED_LIBS=OFF
  )

  ExternalProject_Get_Property(fmt_ext INSTALL_DIR)

  file(MAKE_DIRECTORY "${INSTALL_DIR}/include")

  add_library(fmt STATIC IMPORTED)
  add_dependencies(fmt fmt_ext)
  set_target_properties(fmt PROPERTIES
    INTERFACE_INCLUDE_DIRECTORIES "${INSTALL_DIR}/include"
    IMPORTED_LOCATION_DEBUG "${INSTALL_DIR}/lib/${CMAKE_STATIC_LIBRARY_PREFIX}fmtd${CMAKE_STATIC_LIBRARY_SUFFIX}"
    IMPORTED_LOCATION_RELEASE "${INSTALL_DIR}/lib/${CMAKE_STATIC_LIBRARY_PREFIX}fmt${CMAKE_STATIC_LIBRARY_SUFFIX}"
  )

  add_library(fmt::fmt ALIAS fmt)
endif()
