// gui.h
//
// Copyright (C) 2025-present, the Celestia Development Team
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#pragma once

#include <memory>
#include <string>
#include <vector>

#include <imgui.h>
#include <SDL_events.h>
#include <SDL_video.h>

#include <celestia/scriptmenu.h>

class CelestiaCore;

namespace celestia::sdl
{

class AboutDialog;
class Environment;
class TimeDialog;

class Gui //NOSONAR
{
    struct Private { explicit Private() = default; };

public:
    Gui(Private, ImGuiContext*, ImGuiIO*, CelestiaCore*, std::string&&);
    ~Gui();

    static std::unique_ptr<Gui> create(SDL_Window*,
                                       SDL_GLContext,
                                       CelestiaCore*,
                                       const Environment&);

    void processEvent(const SDL_Event&) const;
    void render();

    inline bool wantCaptureKeyboard() const { return m_io->WantCaptureKeyboard; }
    inline bool wantCaptureMouse() const { return m_io->WantCaptureMouse; }
    inline bool isQuitRequested() const { return m_isQuitRequested; }

private:
    void menuBar();
    void scriptMenu();

    CelestiaCore* m_appCore;
    ImGuiContext* m_ctx;
    ImGuiIO* m_io;
    std::string m_iniFilename;

    std::vector<ScriptMenuItem> m_scripts{ ScanScriptsDirectory("scripts", false) };

    std::unique_ptr<AboutDialog> m_aboutDialog;
    std::unique_ptr<TimeDialog> m_timeDialog;

    bool m_isAboutDialogOpen{ false };
    bool m_isObjectsDialogOpen{ false };
    bool m_isRenderDialogOpen{ false };
    bool m_isTimeDialogOpen{ false };

    float m_menuBarHeight{ 0.0f };

    bool m_isQuitRequested{ false };
};

} // end namespace celestia::sdl
