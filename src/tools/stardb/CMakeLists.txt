# not building celdat2txt as in references external function
foreach(tool makestardb makexindex startextdump)
  add_executable(${tool} "${tool}.cpp")
  target_link_libraries(${tool} ${CELESTIA_LIBS})
  install(TARGETS ${tool} RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
endforeach()
