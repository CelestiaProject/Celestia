macro(build_cmod_tool)
  if(${ARGC} LESS 1)
    message(FATAL_ERROR "build_cmod_tool requires at least 1 argument")
  endif()

  set(__source0 ${ARGV0})
  set(__sources ${ARGV})
  list(REMOVE_AT __sources 0 0)

  include_directories("../common")

  add_executable(${__source0} ${__source0}.cpp ${__sources})
  target_link_libraries(${__source0} celmodel cel3ds cmodcommon)
  add_dependencies(${__source0} celmodel cel3ds cmodcommon)

  install(TARGETS ${__source0} RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
endmacro()

add_subdirectory(common)
add_subdirectory(3dstocmod)
add_subdirectory(cmodfix)
add_subdirectory(cmodsphere)
add_subdirectory(cmodview)
add_subdirectory(itokawa)
