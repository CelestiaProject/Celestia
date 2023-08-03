foreach(tool makestardb makexindex startextdump)
  add_executable(${tool} "${tool}.cpp")
  target_link_libraries(${tool} celestia)
  install(
    TARGETS ${tool}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    COMPONENT tools
  )
endforeach()
