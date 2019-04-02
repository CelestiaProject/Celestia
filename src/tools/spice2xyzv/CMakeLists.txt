if (NOT ENABLE_SPICE)
  return()
endif()

add_executable(spice2xyzv "spice2xyzv.cpp")
target_link_libraries(spice2xyzv CSPICE::CSPICE)
