include(doctest/doctest.cmake)

add_library(doctest_main OBJECT doctest_main.cpp)
target_compile_definitions(doctest_main PUBLIC DOCTEST_CONFIG_USE_STD_HEADERS)
target_include_directories(doctest_main PUBLIC doctest)

set(_CELESTIA_TESTS_BASE_DIR "${CMAKE_CURRENT_LIST_DIR}")

macro(test_case)
  set(tgt ${ARGV0})
  set(tgt_sources ${ARGV})
  list(REMOVE_AT tgt_sources 0)

  add_executable(${tgt} $<TARGET_OBJECTS:doctest_main> ${tgt_sources})
  target_link_libraries(${tgt} PRIVATE celestia)
  if(NOT MSVC)
    target_compile_options(${tgt} PRIVATE $<$<COMPILE_LANGUAGE:CXX>:-fexceptions>)
  endif()
  target_compile_definitions(${tgt} PRIVATE
    DOCTEST_CONFIG_USE_STD_HEADERS
    DOCTEST_CONFIG_NO_TRY_CATCH_IN_ASSERTS)
  target_include_directories(${tgt} PRIVATE "${_CELESTIA_TESTS_BASE_DIR}/doctest")
  if(WIN32)
    # Need to copy the dlls before test discovery
    add_custom_command(
      TARGET ${tgt} PRE_LINK
      COMMAND
        ${CMAKE_COMMAND}
          -D "DEPS_TARGET_FILE=$<TARGET_FILE:celestia>"
          -D "DEPS_DEST_DIR=$<TARGET_FILE_DIR:${tgt}>"
          -D "DEPS_COPY_TARGET=ON"
          -P "${CMAKE_SOURCE_DIR}/cmake/CopyDependentDlls.cmake"
      VERBATIM
    )
  endif()

  doctest_discover_tests(${tgt})
endmacro()

add_subdirectory(integration)
add_subdirectory(unit)
