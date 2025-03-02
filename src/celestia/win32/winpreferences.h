// winpreferences.h
//
// Copyright (C) 2023, Celestia Development Team
//
// Extracted from winmain.cpp:
// Copyright (C) 2001-2007, Chris Laurel <claurel@shatters.net>
//
// The main application window.
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#pragma once

#include <cstdint>
#include <string>

#include <celengine/body.h>
#include <celengine/multitexture.h>
#include <celengine/renderflags.h>
#include <celengine/starcolors.h>

#include <windows.h>

class CelestiaCore;

namespace celestia::win32
{

struct AppPreferences
{
    // Specify some default values in case registry keys are not found. Ideally, these
    // defaults should never be used--they should be overridden by settings in
    // celestia.cfg.
    int winWidth{ 800 };
    int winHeight{ 600 };
    int winX{ CW_USEDEFAULT };
    int winY{ CW_USEDEFAULT };
    RenderFlags renderFlags{ RenderFlags::DefaultRenderFlags };
    RenderLabels labelMode{ RenderLabels::NoLabels };
    std::uint64_t locationFilter{ 0 };
    BodyClassification orbitMask{ BodyClassification::Planet | BodyClassification::Moon };
    float visualMagnitude{ 8.0f };
    float ambientLight{ 0.1f }; // Low
    float galaxyLightGain{ 0.0f };
    int showLocalTime{ 0 };
    int dateFormat{ 0 };
    int hudDetail{ 2 };
    int fullScreenMode{ -1 };
    int starsColor{ static_cast<int>(ColorTableType::Blackbody_D65) };
    std::uint32_t lastVersion{ 0 };
    std::string altSurfaceName;
    TextureResolution textureResolution{ TextureResolution::medres };
    StarStyle starStyle{ StarStyle::PointStars };
#ifndef PORTABLE_BUILD
    bool ignoreOldFavorites{ false };
#endif
};

bool LoadPreferencesFromRegistry(AppPreferences& prefs);
bool SavePreferencesToRegistry(AppPreferences& prefs);

}
