function(EnableWinICU)
  # https://learn.microsoft.com/en-us/windows/win32/intl/international-components-for-unicode--icu-
  try_compile(
    HAVE_WIN_ICU_1903
    ${CMAKE_BINARY_DIR}
    "${CMAKE_SOURCE_DIR}/checks/winicunew.cpp"
    LINK_LIBRARIES icu
  )
  if(HAVE_WIN_ICU_1903)
    add_definitions(-DHAVE_WIN_ICU_COMBINED_HEADER)
    link_libraries("icu")
    message(STATUS "Found ICU with combined header and combined lib")
  else()
    try_compile(
      HAVE_WIN_ICU_1709
      ${CMAKE_BINARY_DIR}
      "${CMAKE_SOURCE_DIR}/checks/winicunew.cpp"
      LINK_LIBRARIES icuuc icuin
    )
    if(HAVE_WIN_ICU_1709)
      add_definitions(-DHAVE_WIN_ICU_COMBINED_HEADER)
      link_libraries("icuuc" "icuin")
      message(STATUS "Found ICU with combined header and separate libs")
    else()
      try_compile(
        HAVE_WIN_ICU_1703
        ${CMAKE_BINARY_DIR}
        "${CMAKE_SOURCE_DIR}/checks/winicuold.cpp"
        LINK_LIBRARIES icuuc icuin
      )
      if(HAVE_WIN_ICU_1703)
        add_definitions(-DHAVE_WIN_ICU_SEPARATE_HEADERS)
        link_libraries("icuuc" "icuin")
        message(STATUS "Found ICU with separate headers and separate libs")
      else()
        message(FATAL_ERROR "Unable to find Windows SDK's ICU implementation")
      endif()
    endif()
  endif()
endfunction()
