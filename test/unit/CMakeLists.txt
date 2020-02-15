include(TestCase)

test_case(hash celengine)
test_case(fs celengine)
if(WIN32)
  test_case(winutil celutil)
endif()
