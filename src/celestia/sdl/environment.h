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

#include <SDL_video.h>

namespace celestia::sdl
{

class AppWindow;
class Settings;

class Environment : public std::enable_shared_from_this<Environment> //NOSONAR
{
    struct Private { explicit Private() = default; };

public:
    explicit Environment(Private) {}
    ~Environment();

    Environment(const Environment&) = delete;
    Environment& operator=(const Environment&) = delete;
    Environment(Environment&&) = delete;
    Environment& operator=(Environment&&) = delete;

    static std::shared_ptr<Environment> init();

    bool setGLAttributes() const;
    std::unique_ptr<AppWindow> createAppWindow(const Settings&);

    std::filesystem::path getSettingsPath() const;
    std::filesystem::path getImguiSettingsPath() const;
};

} // end namespace celestia::sdl
