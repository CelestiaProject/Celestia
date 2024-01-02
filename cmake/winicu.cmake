function(EnableWinICU)
  # https://learn.microsoft.com/en-us/windows/win32/intl/international-components-for-unicode--icu-
  try_compile(
    HAVE_WIN_ICU_1903
    ${CMAKE_BINARY_DIR}
    "${CMAKE_SOURCE_DIR}/checks/winicunew.cpp"
    COMPILE_DEFINITIONS
      -D_WIN32_WINNT=_WIN32_WINNT_WIN10
      -DNTDDI_VERSION=NTDDI_WIN10_19H1
    LINK_LIBRARIES icu
  )
  # _WIN32_WINNT and NTDDI_VERSION need to be set to enable the icu header to work properly.
  # https://learn.microsoft.com/en-gb/windows/win32/winprog/using-the-windows-headers
  add_definitions(-D_WIN32_WINNT=_WIN32_WINNT_WIN10)
  if(HAVE_WIN_ICU_1903)
    add_definitions(-DHAVE_WIN_ICU_COMBINED_HEADER -DNTDDI_VERSION=NTDDI_WIN10_19H1)
    link_libraries("icu")
    message(STATUS "Found ICU with combined header and combined lib")
  else()
    try_compile(
      HAVE_WIN_ICU_1709
      ${CMAKE_BINARY_DIR}
      "${CMAKE_SOURCE_DIR}/checks/winicunew.cpp"
      COMPILE_DEFINITIONS
        -D_WIN32_WINNT=_WIN32_WINNT_WIN10
        -DNTDDI_VERSION=NTDDI_WIN10_RS3
      LINK_LIBRARIES icuuc icuin
    )
    if(HAVE_WIN_ICU_1709)
      add_definitions(-DHAVE_WIN_ICU_COMBINED_HEADER -DNTDDI_VERSION=NTDDI_WIN10_RS3)
      link_libraries("icuuc" "icuin")
      message(STATUS "Found ICU with combined header and separate libs")
    else()
      try_compile(
        HAVE_WIN_ICU_1703
        ${CMAKE_BINARY_DIR}
        "${CMAKE_SOURCE_DIR}/checks/winicuold.cpp"
        COMPILE_DEFINITIONS
          -D_WIN32_WINNT=_WIN32_WINNT_WIN10
          -DNTDDI_VERSION=NTDDI_WIN10_RS2
        LINK_LIBRARIES icuuc icuin
      )
      if(HAVE_WIN_ICU_1703)
        add_definitions(-DHAVE_WIN_ICU_SEPARATE_HEADERS -DNTDDI_VERSION=NTDDI_WIN10_RS2)
        link_libraries("icuuc" "icuin")
        message(STATUS "Found ICU with separate headers and separate libs")
      else()
        message(FATAL_ERROR "Unable to find Windows SDK's ICU implementation")
      endif()
    endif()
  endif()
endfunction()
