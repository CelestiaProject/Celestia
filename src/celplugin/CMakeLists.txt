cmake_minimum_required(VERSION 3.7)
if (NOT WIN32)
  add_compile_options(-fPIC)
  add_compile_options(-g)
else()
  add_definitions(
	  -D_CRT_SECURE_NO_WARNINGS
	  -D_SCL_SECURE_NO_WARNINGS
	  -DNOMINMAX
	  -DWIN32_LEAN_AND_MEAN
  )
endif()
find_package(fmt 4.0.0 CONFIG REQUIRED)
if (WIN32)
  set(WINUTIL ../celutil/winutil.cpp)
endif()
add_executable(plugintest host.cpp plugin.cpp pluginmanager.cpp ../celcompat/fs.cpp ../celutil/util.cpp ${WINUTIL})
include_directories(. ..)
target_link_libraries(plugintest fmt::fmt ${CMAKE_DL_LIBS})
add_library(myplug SHARED myplug.cpp)
