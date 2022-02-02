test_case(3ds_load)
test_case(cmod_bin_ascii_roundtrip)

file(COPY "${CMAKE_SOURCE_DIR}/test/data/huygens.3ds"
     DESTINATION "${CMAKE_CURRENT_BINARY_DIR}")
file(COPY "${CMAKE_SOURCE_DIR}/test/data/iss/models/iss.cmod"
     DESTINATION "${CMAKE_CURRENT_BINARY_DIR}")
