foreach(tool xyzv2bin bin2xyzv)
  add_executable(${tool} "${tool}.cpp")
  install(TARGETS ${tool} RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
endforeach()

install_perl_tools(xyzv2bin.pl)
