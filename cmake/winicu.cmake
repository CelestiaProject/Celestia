function(EnableWinICU)
  # https://learn.microsoft.com/en-us/windows/win32/intl/international-components-for-unicode--icu-
  # https://learn.microsoft.com/en-us/windows/win32/winprog/using-the-windows-headers

  # Combined header/library - requires Windows 10 1903
  try_compile(
    HAVE_WIN_ICU_1903
    ${CMAKE_BINARY_DIR}
    "${CMAKE_SOURCE_DIR}/checks/winicunew.cpp"
    LINK_LIBRARIES icu
  )
  set(WINVER 0x0A00 PARENT_SCOPE)
  if(HAVE_WIN_ICU_1903)
    set(NTDDI_VER 0x0A000007 PARENT_SCOPE)
    add_definitions(-DHAVE_WIN_ICU_COMBINED_HEADER)
    link_libraries("icu")
    message(STATUS "Found ICU with combined header and combined lib")
    return()
  endif()

  # Combined header/separate libraries - requires Windows 10 1709
  try_compile(
    HAVE_WIN_ICU_1709
    ${CMAKE_BINARY_DIR}
    "${CMAKE_SOURCE_DIR}/checks/winicunew.cpp"
    LINK_LIBRARIES icuuc icuin
  )
  if(HAVE_WIN_ICU_1709)
    set(NTDDI_VER 0x0A000004 PARENT_SCOPE)
    add_definitions(-DHAVE_WIN_ICU_COMBINED_HEADER)
    link_libraries("icuuc" "icuin")
    message(STATUS "Found ICU with combined header and separate libs")
    return()
  endif()

  # Separate headers/separate libraries - requires Windows 10 1703
  try_compile(
    HAVE_WIN_ICU_1703
    ${CMAKE_BINARY_DIR}
    "${CMAKE_SOURCE_DIR}/checks/winicuold.cpp"
    LINK_LIBRARIES icuuc icuin
  )
  if(HAVE_WIN_ICU_1703)
    set(NTDDI_VER 0x0A000003 PARENT_SCOPE)
    add_definitions(-DHAVE_WIN_ICU_COMBINED_HEADER)
    link_libraries("icuuc" "icuin")
    message(STATUS "Found ICU with separate headers and separate libs")
  else()
    message(FATAL_ERROR "Unable to find Windows SDK's ICU implementation")
  endif()
endfunction()
