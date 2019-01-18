if(NOT fmt_FOUND)
  list(APPEND FMT_SOURCES
    ${CMAKE_SOURCE_DIR}/thirdparty/fmt/src/format.cc
    ${CMAKE_SOURCE_DIR}/thirdparty/fmt/src/posix.cc
  )
endif()

foreach(tool xyzv2bin bin2xyzv)
  add_executable(${tool} "${tool}.cpp" ${FMT_SOURCES})
  install(TARGETS ${tool} RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
endforeach()
