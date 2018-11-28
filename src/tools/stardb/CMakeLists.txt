# not building celdat2txt as in references external function
foreach(tool makestardb makexindex startextdump)
  add_executable(${tool} WIN32 "${tool}.cpp")
  target_link_libraries(${tool} ${CELESTIA_LIBS})
  install(TARGETS ${tool} RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
endforeach()
