// environment.cpp
//
// Copyright (C) 2020-present, the Celestia Development Team
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include "environment.h"

#include <cstdio>
#include <fstream>
#include <string_view>
#include <system_error>

#include <fmt/format.h>
#include <fmt/ostream.h>
#include <SDL.h>

#include "appwindow.h"
#include "gui.h"
#include "helpers.h"
#include "settings.h"

using namespace std::string_view_literals;

namespace celestia::sdl
{

namespace
{

UniqueSDL<char>
getPrefsDirectory()
{
    return UniqueSDL<char>{ SDL_GetPrefPath("Celestia Project", "Celestia") };
}

} // end unnamed namespace

Environment::~Environment()
{
    SDL_Quit();
}

std::shared_ptr<Environment>
Environment::init()
{
    static std::weak_ptr<Environment> g_environment;
    if (auto environment = g_environment.lock(); environment != nullptr)
        return environment;

    if (SDL_Init(SDL_INIT_VIDEO) != 0)
    {
        fatalError("Failed to initialize SDL: {}", SDL_GetError());
        return nullptr;
    }

    auto environment = std::make_shared<Environment>(Private{});
    g_environment = environment;
    return environment;
}

bool
Environment::setGLAttributes() const
{
    if (SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1) != 0)
    {
        fatalError("Failed to set double buffering: {}", SDL_GetError());
        return false;
    }

    if (SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24) != 0)
    {
        fatalError("Failed to set depth size: {}", SDL_GetError());
        return false;
    }

#ifdef GL_ES
    if (SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES) != 0)
    {
        fatalError("Failed to set OpenGL context: {}", SDL_GetError());
        return false;
    }

    if (SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2) != 0
        || SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0) != 0)
    {
        fatalError("Failed to set context version: {}", SDL_GetError());
        return false;
    }
#endif

    return true;
}

std::unique_ptr<AppWindow>
Environment::createAppWindow(const Settings& settings)
{
    Uint32 flags = SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI;
    if (settings.isFullscreen)
        flags |= SDL_WINDOW_FULLSCREEN_DESKTOP;

    UniqueWindow window{ SDL_CreateWindow("Celestia",
                                          settings.positionX, settings.positionY,
                                          settings.width, settings.height,
                                          flags) };
    if (!window)
    {
        fatalError("Could not create Window: {}", SDL_GetError());
        return nullptr;
    }

    UniqueGLContext context{ SDL_GL_CreateContext(window.get()) };
    if (context.get() == nullptr)
    {
        fatalError("Could not create OpenGL context: {}", SDL_GetError());
        return nullptr;
    }

    // First try to enable adaptive sync and then vsync
    if (SDL_GL_SetSwapInterval(-1) == -1)
        SDL_GL_SetSwapInterval(1);

    gl::init();
#ifndef GL_ES
    if (!gl::checkVersion(gl::GL_2_1))
    {
        fatalError("Celestia requires OpenGL 2.1");
        return nullptr;
    }
#endif

    return std::make_unique<AppWindow>(AppWindow::Private{},
                                       shared_from_this(),
                                       std::move(window),
                                       std::move(context),
                                       settings.isFullscreen);
}

fs::path
Environment::getSettingsPath() const
{
    UniqueSDL<char> prefsPath = getPrefsDirectory();
    if (prefsPath == nullptr)
        return {};
    return fs::u8path(prefsPath.get()) / "sdlsettings.dat";
}

fs::path
Environment::getImguiSettingsPath() const
{
    UniqueSDL<char> prefsPath = getPrefsDirectory();
    if (prefsPath == nullptr)
        return {};
    return fs::u8path(prefsPath.get()) / "imguisettings.ini";
}

} // end namespace celestia::sdl
