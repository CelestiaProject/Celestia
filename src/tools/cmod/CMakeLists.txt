macro(build_cmod_tool)
  if(${ARGC} LESS 1)
    message(FATAL_ERROR "build_cmod_tool requires at least 1 argument")
  endif()

  set(__target ${ARGV0})
  set(__win32  ${ARGV1})
  set(__sources ${ARGV})

  if("${__win32}" STREQUAL "WIN32")
    list(REMOVE_AT __sources 0 1)
  else()
    list(REMOVE_AT __sources 0 0)
    set(__win32)
  endif()

  list(LENGTH __sources len)
  if(${len} EQUAL 0)
    set(__sources "${__target}.cpp")
  endif()

  include_directories("../common")

  add_executable(${__target} ${__win32} ${__sources})
  target_link_libraries(${__target} celestia cmodcommon)
  add_dependencies(${__target} celestia cmodcommon)

  install(TARGETS ${__target} RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
endmacro()

add_subdirectory(common)
add_subdirectory(3dstocmod)
add_subdirectory(cmodfix)
add_subdirectory(cmodsphere)
add_subdirectory(cmodview)
add_subdirectory(itokawa)
