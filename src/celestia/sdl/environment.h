// environment.h
//
// Copyright (C) 2020-present, the Celestia Development Team
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#pragma once

#include <filesystem>
#include <memory>
#include <optional>

#include <SDL_video.h>

#include <celestia/celestiacore.h>

namespace celestia::sdl
{

class Alerter;
class AppWindow;
class Settings;

class Environment : public std::enable_shared_from_this<Environment>, public CelestiaCore::CursorHandler //NOSONAR
{
    struct Private { explicit Private() = default; };

public:
    explicit Environment(Private) {}
    ~Environment() override;

    Environment(const Environment&) = delete;
    Environment& operator=(const Environment&) = delete;
    Environment(Environment&&) = delete;
    Environment& operator=(Environment&&) = delete;

    static std::shared_ptr<Environment> init();

    void setCursorShape(CelestiaCore::CursorShape) override
    {
        // ImGui manages the SDL cursor shape.
    }
    CelestiaCore::CursorShape getCursorShape() const override { return CelestiaCore::ArrowCursor; }
#ifdef __EMSCRIPTEN__
    bool warpCursorRequested() const { return false; }
#else
    bool warpCursorRequested() const { return m_warpCursor; }
    void setWarpCursor(bool enabled) { m_warpCursor = enabled; }
#endif
    bool warpsCursor() const override { return m_dragWarp.value_or(warpCursorRequested()); }
    void beginDrag() { m_dragWarp = warpCursorRequested(); }
    void endDrag() { m_dragWarp.reset(); }

    bool setGLAttributes(int aaSamples) const;
    std::unique_ptr<AppWindow> createAppWindow(const Settings&,
                                               std::unique_ptr<CelestiaCore>&&,
                                               std::unique_ptr<Alerter>&&);

    std::filesystem::path getSettingsPath() const;
    std::filesystem::path getImguiSettingsPath() const;

private:
    std::optional<bool> m_dragWarp;
#ifndef __EMSCRIPTEN__
    bool m_warpCursor{ true };
#endif
};

} // end namespace celestia::sdl
