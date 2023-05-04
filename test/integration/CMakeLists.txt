set(INTEGRATION_TEST_SOURCES
  3ds_load_test.cpp
  cmod_bin_ascii_roundtrip_test.cpp)

test_case(integration "${INTEGRATION_TEST_SOURCES}")

file(COPY "${CMAKE_SOURCE_DIR}/test/data/huygens.3ds"
     DESTINATION "${CMAKE_CURRENT_BINARY_DIR}")
file(COPY "${CMAKE_SOURCE_DIR}/test/data/iss/models/iss.cmod"
     DESTINATION "${CMAKE_CURRENT_BINARY_DIR}")
