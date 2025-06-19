// gui.cpp
//
// Copyright (C) 2025-present, the Celestia Development Team
//
// Based on the Qt interface
// Copyright (C) 2005-2008, the Celestia Development Team
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include "gui.h"

#include <algorithm>

#include <celengine/glsupport.h>

#include <imgui_impl_opengl3.h>
#include <imgui_impl_sdl2.h>

#include <celengine/simulation.h>
#include <celestia/celestiacore.h>
#include <celutil/stringutils.h>
#include "aboutdialog.h"
#include "clipboard.h"
#include "environment.h"
#include "objectsdialog.h"
#include "renderdialog.h"
#include "timedialog.h"

#ifdef _WIN32
#include <celutil/winutil.h>
#endif

namespace celestia::sdl
{

Gui::Gui(Private, ImGuiContext* ctx, ImGuiIO* io, CelestiaCore* appCore, std::string&& iniFilename) :
    m_appCore(appCore),
    m_ctx(ctx),
    m_io(io),
    m_iniFilename(std::move(iniFilename))
{
    std::sort(m_scripts.begin(), m_scripts.end(),
              [](const auto& a, const auto& b)
              {
                  return compareIgnoringCase(a.title, b.title) < 0;
              });
}

Gui::~Gui()
{
    if (!m_iniFilename.empty())
        ImGui::SaveIniSettingsToDisk(m_iniFilename.c_str());

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext(m_ctx);
}

std::unique_ptr<Gui>
Gui::create(SDL_Window* window, SDL_GLContext context, CelestiaCore* appCore, const Environment& environment)
{
    if (!IMGUI_CHECKVERSION())
        return nullptr;

    ImGuiContext* ctx = ImGui::CreateContext();
    if (ctx == nullptr)
        return nullptr;

    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.IniFilename = nullptr;

    fs::path iniPath = environment.getImguiSettingsPath();
#ifdef _WIN32
    std::string iniFilename = util::WideToUTF8(iniPath.native());
#else
    std::string iniFilename = iniPath.string();
#endif

    if (!iniFilename.empty())
        ImGui::LoadIniSettingsFromDisk(iniFilename.c_str());

    ImGui::StyleColorsDark();

    if (!ImGui_ImplSDL2_InitForOpenGL(window, context))
    {
        ImGui::DestroyContext(ctx);
        return nullptr;
    }

    if (!ImGui_ImplOpenGL3_Init())
    {
        ImGui_ImplSDL2_Shutdown();
        ImGui::DestroyContext(ctx);
        return nullptr;
    }

    return std::make_unique<Gui>(Private{}, ctx, &io, appCore, std::move(iniFilename));
}

void
Gui::processEvent(const SDL_Event& event) const
{
    ImGui_ImplSDL2_ProcessEvent(&event);
}

void
Gui::render()
{
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplSDL2_NewFrame();
    ImGui::NewFrame();

    menuBar();

    objectsDialog(m_appCore, &m_isObjectsDialogOpen);
    renderDialog(m_appCore, &m_isRenderDialogOpen);
    if (m_isAboutDialogOpen)
    {
        if (m_aboutDialog == nullptr)
            m_aboutDialog = std::make_unique<AboutDialog>();
        m_aboutDialog->show(&m_isAboutDialogOpen);
    }

    if (m_isTimeDialogOpen)
    {
        if (m_timeDialog == nullptr)
            m_timeDialog = std::make_unique<TimeDialog>(m_appCore);
        m_timeDialog->show(&m_isTimeDialogOpen);
    }

    ImGui::Render();
    glViewport(0, 0, static_cast<GLsizei>(m_io->DisplaySize.x), static_cast<GLsizei>(m_io->DisplaySize.y));
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void
Gui::menuBar()
{
    ImGui::BeginMainMenuBar();

    if (float menuBarHeight = ImGui::GetFrameHeight(); menuBarHeight != m_menuBarHeight)
    {
        m_appCore->setSafeAreaInsets(0, static_cast<int>(menuBarHeight), 0, 0);
        m_menuBarHeight = menuBarHeight;
    }

    if (ImGui::BeginMenu("File"))
    {
        if (ImGui::BeginMenu("Scripts..."))
        {
            scriptMenu();
            ImGui::EndMenu();
        }

        ImGui::Separator();

        if (ImGui::MenuItem("Objects..."))
            m_isObjectsDialogOpen = true;
        if (ImGui::MenuItem("Render..."))
            m_isRenderDialogOpen = true;

        ImGui::Separator();
        if (ImGui::MenuItem("Exit"))
            m_isQuitRequested = true;

        ImGui::EndMenu();
    }

    if (ImGui::BeginMenu("Navigation"))
    {
        if (ImGui::MenuItem("Set time...") && !m_isTimeDialogOpen)
        {
            const Observer* observer = m_appCore->getSimulation()->getActiveObserver();
            if (m_timeDialog == nullptr)
                m_timeDialog = std::make_unique<TimeDialog>(m_appCore);
            m_timeDialog->setTime(observer->getTime());
            m_isTimeDialogOpen = true;
        }

        ImGui::Separator();

        if (ImGui::MenuItem("Copy URL##copyCommand", "CTRL+C"))
            doCopy(*m_appCore);
        if (ImGui::MenuItem("Paste URL##pasteCommand", "CTRL+V"))
            doPaste(*m_appCore);
        ImGui::EndMenu();
    }

    if (ImGui::BeginMenu("Help"))
    {
        if (ImGui::MenuItem("About Celestia..."))
            m_isAboutDialogOpen = true;
        ImGui::EndMenu();
    }

    ImGui::EndMainMenuBar();
}

void
Gui::scriptMenu()
{
    for (const auto& item : m_scripts)
    {
        if (ImGui::MenuItem(item.title.c_str()))
            m_appCore->runScript(item.filename);
    }
}

} // end namespace celestia::sdl
