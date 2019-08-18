add_executable(test-utf8 utf8.cpp)
target_link_libraries(test-utf8 celutil)

add_executable(test-name name.cpp)
target_link_libraries(test-name celengine celephem celutil celtxf)

add_executable(test-greek greek.cpp)
target_link_libraries(test-greek celutil celcompat)

add_executable(test-astrodb astrodb.cpp)
target_link_libraries(test-astrodb celengine celutil celephem celtxf celcompat)

add_executable(test-octree octree.cpp)
target_link_libraries(test-octree celengine celutil celephem celtxf)

add_executable(test-frustum frustum.cpp)
target_link_libraries(test-frustum celengine celutil celephem celtxf)

add_executable(test-crossindex crossindex.cpp)
target_link_libraries(test-crossindex celengine celutil celephem celtxf celcompat)

add_executable(test-astroobj astroobj.cpp)
target_link_libraries(test-astroobj celengine celephem celutil celtxf)
