include(TestCase)

test_case(hash)
test_case(fs)
test_case(stellarclass)
if(WIN32)
  test_case(winutil)
endif()
