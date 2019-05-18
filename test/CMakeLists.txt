add_executable(utf8 utf8.cpp)
target_link_libraries(utf8 celutil)

add_executable(name name.cpp)
target_link_libraries(name celengine celutil)

add_executable(greek greek.cpp)
target_link_libraries(greek celutil)

add_executable(astrodb astrodb.cpp)
target_link_libraries(astrodb celengine celutil celephem celtxf)

add_executable(octree octree.cpp)
target_link_libraries(octree celengine celutil)

add_executable(frustum frustum.cpp)
target_link_libraries(frustum celengine celutil)

add_executable(crossindex crossindex.cpp)
target_link_libraries(crossindex celengine celutil)
