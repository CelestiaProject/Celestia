test_case(cmod_bin_ascii_roundtrip)

file(COPY "${CMAKE_SOURCE_DIR}/test/data/iss/models/iss.cmod"
     DESTINATION "${CMAKE_CURRENT_BINARY_DIR}")
