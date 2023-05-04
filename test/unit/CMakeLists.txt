set(UNIT_TEST_SOURCES
  array_view_test.cpp
  arrayvector_test.cpp
  category_test.cpp
  greek_test.cpp
  hash_test.cpp
  intrusiveptr_test.cpp
  logger_test.cpp
  stellarclass_test.cpp
  tokenizer_test.cpp)

#if(NOT HAVE_FLOAT_CHARCONV)
  list(APPEND UNIT_TEST_SOURCES charconv_compat_test.cpp)
#endif()

if(USE_ICU)
  list(APPEND UNIT_TEST_SOURCES unicode_test.cpp)
endif()

if(WIN32)
  list(APPEND UNIT_TEST_SOURCES winutil_test.cpp)
endif()

test_case(unit "${UNIT_TEST_SOURCES}")
