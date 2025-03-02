// appwindow.h
//
// Copyright (C) 2020-present, the Celestia Development Team
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#pragma once

#include <memory>

#include <celengine/glsupport.h>
#include <SDL_events.h>
#include <SDL_stdinc.h>
#include <SDL_video.h>

#include <celutil/uniquedel.h>
#include "environment.h"
#include "glcontext.h"

class CelestiaCore;

namespace celestia::sdl
{

using UniqueWindow = util::UniquePtrDel<SDL_Window, &SDL_DestroyWindow>;

class Gui;
class Settings;

class AppWindow //NOSONAR
{
    struct Private { explicit Private() = default; };

public:
    AppWindow(Private, const std::shared_ptr<Environment>&, UniqueWindow&&, UniqueGLContext&&, bool);
    ~AppWindow();

    void dumpGLInfo() const;
    bool run(const Settings&);
    bool update();

    void getSize(int& width, int& height) const;
    void getPosition(int& x, int& y) const;
    inline bool isFullscreen() const { return m_isFullscreen; }

private:
    class Alerter;

    void handleTextInputEvent(const SDL_TextInputEvent&);
    void handleKeyDownEvent(const SDL_KeyboardEvent&);
    void handleKeyUpEvent(const SDL_KeyboardEvent&);
    void handleWindowEvent(const SDL_WindowEvent&);
    void handleMouseButtonDownEvent(const SDL_MouseButtonEvent&);
    void handleMouseButtonUpEvent(const SDL_MouseButtonEvent&);
    void handleMouseWheelEvent(const SDL_MouseWheelEvent&);
    void handleMouseMotionEvent(const SDL_MouseMotionEvent&);

    void toggleFullscreen();

    // Important! Members destroyed in reverse order of declaration
    std::shared_ptr<Environment> m_environment;
    UniqueWindow m_window;
    UniqueGLContext m_context;
    std::unique_ptr<CelestiaCore> m_appCore;
    std::unique_ptr<Alerter> m_alerter;
    std::unique_ptr<Gui> m_gui;

    int m_width{ 0 };
    int m_height{ 0 };

    // mouse drag data
    Sint32 m_lastX{ 0 };
    Sint32 m_lastY{ 0 };

    bool m_isFullscreen;

    friend class Environment;
};

} // end namespace celestia::sdl
