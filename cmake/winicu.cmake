function(EnableWinICU)
  # https://learn.microsoft.com/en-us/windows/win32/intl/international-components-for-unicode--icu-
  # https://learn.microsoft.com/en-us/windows/win32/winprog/using-the-windows-headers

  # Combined header/library - requires Windows 10 1903
  try_compile(
    HAVE_WIN_ICU
    ${CMAKE_BINARY_DIR}
    "${CMAKE_SOURCE_DIR}/checks/winicu.cpp"
    LINK_LIBRARIES icu
  )
  set(WINVER 0x0A00 PARENT_SCOPE)
  if(HAVE_WIN_ICU)
    set(NTDDI_VER 0x0A000007 PARENT_SCOPE)
    add_definitions(-DHAVE_WIN_ICU)
    link_libraries("icu")
    message(STATUS "Found ICU with combined header and combined lib")
    return()
  else()
    message(FATAL_ERROR "Unable to find Windows SDK's ICU implementation")
  endif()
endfunction()
