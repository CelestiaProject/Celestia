// alerter.h
//
// Copyright (C) 2020-present, the Celestia Development Team
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#pragma once

#include <string>

#include <SDL_video.h>

#include <celestia/celestiacore.h>

namespace celestia::sdl
{

class Alerter : public CelestiaCore::Alerter
{
public:
    void setWindow(SDL_Window* window) noexcept { m_window = window; }
    void fatalError(const std::string& msg) override;

private:
    SDL_Window* m_window{ nullptr };
};

}
