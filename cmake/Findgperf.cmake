find_program(gperf_EXECUTABLE NAMES gperf)

find_package(PackageHandleStandardArgs)

find_package_handle_standard_args(gperf
  FOUND_VAR gperf_FOUND
  REQUIRED_VARS gperf_EXECUTABLE
)

if(gperf_EXECUTABLE AND NOT TARGET gperf::gperf)
  add_executable(gperf::gperf IMPORTED)
  set_target_properties(gperf::gperf PROPERTIES
    IMPORTED_LOCATION "${gperf_EXECUTABLE}"
  )
endif()

function(gperf_add_table target gperffile src_file num_iters)
  if(NOT TARGET gperf::gperf)
    message(FATAL_ERROR "The gperf executable has not been found on your system")
  endif()
  get_filename_component(BASE_NAME ${gperffile} NAME_WE)
  set(OUTPUT_DIR "${CMAKE_CURRENT_BINARY_DIR}/${target}_autogen")
  set(OUTPUT_PATH "${OUTPUT_DIR}/${BASE_NAME}.inc")
  add_custom_command(
    OUTPUT "${OUTPUT_PATH}"
    COMMAND gperf::gperf "${CMAKE_CURRENT_SOURCE_DIR}/${gperffile}" -m${num_iters} --output-file=${OUTPUT_PATH}
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/${gperffile}
    VERBATIM
  )
  set_property(SOURCE ${src_file} APPEND PROPERTY OBJECT_DEPENDS "${OUTPUT_PATH}")
  target_include_directories(${target} PRIVATE "${OUTPUT_DIR}")
endfunction()
