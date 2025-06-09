if(NOT ENABLE_SDL)
  message(STATUS "SDL frontend is disabled.")
  return()
endif()

find_package(SDL2 CONFIG REQUIRED)

if(USE_IMGUI)
  set(IMGUI_DIR "${PROJECT_SOURCE_DIR}/thirdparty/imgui")

  set(IMGUI_SOURCES
    "${IMGUI_DIR}/imconfig.h"
    "${IMGUI_DIR}/imgui_demo.cpp"
    "${IMGUI_DIR}/imgui_draw.cpp"
    "${IMGUI_DIR}/imgui_internal.h"
    "${IMGUI_DIR}/imgui_tables.cpp"
    "${IMGUI_DIR}/imgui_widgets.cpp"
    "${IMGUI_DIR}/imgui.cpp"
    "${IMGUI_DIR}/imgui.h"
    "${IMGUI_DIR}/imstb_rectpack.h"
    "${IMGUI_DIR}/imstb_textedit.h"
    "${IMGUI_DIR}/imstb_truetype.h"
    "${IMGUI_DIR}/backends/imgui_impl_opengl3.cpp"
    "${IMGUI_DIR}/backends/imgui_impl_opengl3.h"
    "${IMGUI_DIR}/backends/imgui_impl_sdl2.cpp"
    "${IMGUI_DIR}/backends/imgui_impl_sdl2.h"
    "${IMGUI_DIR}/misc/cpp/imgui_stdlib.cpp"
    "${IMGUI_DIR}/misc/cpp/imgui_stdlib.h")

  add_library(imgui STATIC ${IMGUI_SOURCES})
  target_include_directories(imgui PUBLIC "${IMGUI_DIR}" "${IMGUI_DIR}/backends" "${IMGUI_DIR}/misc/cpp")

  set(SDL_SOURCES
    aboutdialog.cpp
    aboutdialog.h
    appwindow.cpp
    appwindow.h
    clipboard.cpp
    clipboard.h
    environment.cpp
    environment.h
    glcontext.cpp
    glcontext.h
    gui.cpp
    gui.h
    helpers.h
    objectsdialog.cpp
    objectsdialog.h
    renderdialog.cpp
    renderdialog.h
    sdlmain-imgui.cpp
    settings.cpp
    settings.h
    timedialog.cpp
    timedialog.h)

  add_executable(celestia-sdl ${SDL_SOURCES})
  gperf_add_table(celestia-sdl settings.gperf settings.cpp 4)
  add_dependencies(celestia-sdl celestia)
  target_link_libraries(celestia-sdl PRIVATE imgui celestia)
else()
  set(SDL_SOURCES sdlmain.cpp)
  add_executable(celestia-sdl ${SDL_SOURCES})
  add_dependencies(celestia-sdl celestia)
  target_link_libraries(celestia-sdl PRIVATE celestia)
endif()

set_target_properties(celestia-sdl PROPERTIES CXX_VISIBILITY_PRESET hidden)

if(TARGET SDL2::SDL2main AND TARGET SDL2::SDL2)
  if(MINGW)
    target_compile_definitions(celestia-sdl PRIVATE SDL_MAIN_HANDLED)
  endif()
  target_link_libraries(celestia-sdl PRIVATE SDL2::SDL2main SDL2::SDL2)
  if(USE_IMGUI)
      target_link_libraries(imgui PUBLIC SDL2::SDL2)
  endif()
else()
  target_include_directories(celestia-sdl PRIVATE ${SDL2_INCLUDE_DIRS})
  target_link_libraries(celestia-sdl PRIVATE ${SDL2_LIBRARIES})
  if(USE_IMGUI)
    target_link_libraries(imgui PUBLIC ${SDL2_LIBRARIES})
  endif()
endif()

install(
  TARGETS celestia-sdl
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
  COMPONENT sdlgui
)

add_subdirectory(data)
