foreach(tool scattersim scattertable)
  add_executable(${tool} "${tool}.cpp")
  install(TARGETS ${tool} RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
endforeach()
