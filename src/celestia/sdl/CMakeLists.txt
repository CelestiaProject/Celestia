if(NOT ENABLE_SDL)
  message(STATUS "SDL frontend is disabled.")
  return()
endif()

find_package(SDL2 CONFIG REQUIRED)

set(SDL_SOURCES sdlmain.cpp)

add_executable(celestia-sdl ${SDL_SOURCES})
add_dependencies(celestia-sdl celestia)
target_link_libraries(celestia-sdl PRIVATE celestia)

set_target_properties(celestia-sdl PROPERTIES CXX_VISIBILITY_PRESET hidden)

if(TARGET SDL2::SDL2main AND TARGET SDL2::SDL2)
  if(MINGW)
    target_compile_definitions(celestia-sdl PRIVATE SDL_MAIN_HANDLED)
  endif()
  target_link_libraries(celestia-sdl PRIVATE SDL2::SDL2main SDL2::SDL2)
else()
  target_include_directories(celestia-sdl PRIVATE ${SDL2_INCLUDE_DIRS})
  target_link_libraries(celestia-sdl PRIVATE ${SDL2_LIBRARIES})
endif()

install(
  TARGETS celestia-sdl
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
  COMPONENT sdlgui
)

add_subdirectory(data)
