# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

if(NOT TARGET miniaudio::miniaudio)
  message(STATUS "Using miniaudio external project download")
  include(ExternalProject)

  ExternalProject_Add(
    miniaudio_ext
    URL "https://github.com/mackron/miniaudio/archive/refs/tags/0.11.17.tar.gz"
    URL_HASH SHA256=4B139065F7068588B73D507D24E865060E942EB731F988EE5A8F1828155B9480
    CONFIGURE_COMMAND ""
    BUILD_COMMAND ""
    TEST_COMMAND ""
    INSTALL_COMMAND ""
  )

  ExternalProject_Get_Property(miniaudio_ext SOURCE_DIR)

  add_library(miniaudio::miniaudio INTERFACE IMPORTED)
  add_dependencies(miniaudio::miniaudio miniaudio_ext)
  set_target_properties(
    miniaudio::miniaudio PROPERTIES
    INTERFACE_INCLUDE_DIRECTORIES "${SOURCE_DIR}"
  )

  if(UNIX AND (NOT APPLE))
    set(THREADS_PREFER_PTHREAD_FLAG ON)
    find_package(Threads REQUIRED)
    target_link_libraries(miniaudio::miniaudio INTERFACE Threads::Threads)
  endif()
endif()
