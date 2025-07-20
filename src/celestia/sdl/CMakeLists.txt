if(NOT ENABLE_SDL)
  message(STATUS "SDL frontend is disabled.")
  return()
endif()

find_package(SDL2 CONFIG REQUIRED)

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
    "${IMGUI_DIR}/backends/imgui_impl_opengl3_loader.h"
    "${IMGUI_DIR}/backends/imgui_impl_opengl3.cpp"
    "${IMGUI_DIR}/backends/imgui_impl_opengl3.h"
    "${IMGUI_DIR}/backends/imgui_impl_sdl2.cpp"
    "${IMGUI_DIR}/backends/imgui_impl_sdl2.h"
    "${IMGUI_DIR}/misc/cpp/imgui_stdlib.cpp"
    "${IMGUI_DIR}/misc/cpp/imgui_stdlib.h")

add_library(imgui STATIC ${IMGUI_SOURCES})
if(ENABLE_GLES)
  target_compile_definitions(imgui PUBLIC IMGUI_IMPL_OPENGL_ES2)
  target_link_libraries(imgui PRIVATE OpenGLES::OpenGLESv2)
else()
  target_link_libraries(imgui PRIVATE OpenGL::GL)
endif()

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
    sdlmain.cpp
    settings.cpp
    settings.h
    timedialog.cpp
    timedialog.h)

add_executable(celestia-sdl ${SDL_SOURCES})

gperf_add_table(celestia-sdl settings.gperf settings.cpp 4)

add_dependencies(celestia-sdl celestia)
target_link_libraries(celestia-sdl PRIVATE imgui celestia)

set_target_properties(celestia-sdl PROPERTIES CXX_VISIBILITY_PRESET hidden)

if(MINGW)
  target_compile_definitions(celestia-sdl PRIVATE SDL_MAIN_HANDLED)
endif()
target_link_libraries(celestia-sdl PRIVATE SDL2::SDL2main SDL2::SDL2)
target_link_libraries(imgui PUBLIC SDL2::SDL2)

if(WIN32)
  install(
    TARGETS celestia-sdl
    RUNTIME_DEPENDENCIES
      PRE_EXCLUDE_REGEXES "^api-ms" "^ext-ms-"
      POST_EXCLUDE_REGEXES ".*system32/.*\\.dll$"
      DIRECTORIES $<TARGET_FILE_DIR:celestia> $<TARGET_FILE_DIR:SDL2::SDL2>
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    COMPONENT sdlgui
  )
else()
  install(
    TARGETS celestia-sdl
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    COMPONENT sdlgui
  )
endif()

add_subdirectory(data)
