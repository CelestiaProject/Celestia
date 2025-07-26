// settings.h
//
// Copyright (C) 2025-present, the Celestia Development Team
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#pragma once

#include <filesystem>

#include <SDL_video.h>

#include <celengine/body.h>
#include <celengine/multitexture.h>
#include <celengine/renderflags.h>
#include <celengine/starcolors.h>

class CelestiaCore;

namespace celestia::sdl
{

class AppWindow;

struct Settings
{
    static Settings load(const std::filesystem::path&);
    static Settings fromApplication(const AppWindow&, const CelestiaCore*);

    bool save(const std::filesystem::path&) const;
    void apply(const CelestiaCore*) const;

    int positionX{ SDL_WINDOWPOS_CENTERED };
    int positionY{ SDL_WINDOWPOS_CENTERED };
    int width{ 640 };
    int height{ 480 };
    bool isFullscreen{ false };
    bool lokTextures{ false };
    RenderFlags renderFlags{ RenderFlags::DefaultRenderFlags };
    RenderLabels labelMode{ RenderLabels::I18nConstellationLabels };
    TextureResolution textureResolution{ TextureResolution::medres };
    BodyClassification orbitMask{ BodyClassification::DefaultOrbitMask };
    int ambientLight{ 0 }; // None
    int tintSaturation{ 50 };
    int minFeatureSize{ 20 };
    ColorTableType starColors{ ColorTableType::SunWhite };
    StarStyle starStyle{ StarStyle::PointStars };
};

}
