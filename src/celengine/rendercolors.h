#pragma once

// rendercolors.h
//
// Copyright (C) 2026-present, the Celestia Development Team
//
// Centralized default colors for the renderer.  All values are authored as
// sRGB.
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include <celutil/color.h>

namespace celestia::engine
{

// A plain aggregate holding one default Color per renderer visual element.
// Construct a fresh set of defaults via RendererColors::defaults(), then
// apply individual members to Renderer::*Color fields as needed.
struct RendererColors
{
    // --- label colors ---
    Color StarLabel;
    Color PlanetLabel;
    Color DwarfPlanetLabel;
    Color MoonLabel;
    Color MinorMoonLabel;
    Color AsteroidLabel;
    Color CometLabel;
    Color SpacecraftLabel;
    Color LocationLabel;
    Color GalaxyLabel;
    Color GlobularLabel;
    Color NebulaLabel;
    Color OpenClusterLabel;
    Color ConstellationLabel;
    Color EquatorialGridLabel;
    Color PlanetographicGridLabel;
    Color GalacticGridLabel;
    Color EclipticGridLabel;
    Color HorizonGridLabel;

    // --- orbit colors ---
    Color StarOrbit;
    Color PlanetOrbit;
    Color DwarfPlanetOrbit;
    Color MoonOrbit;
    Color MinorMoonOrbit;
    Color AsteroidOrbit;
    Color CometOrbit;
    Color SpacecraftOrbit;
    Color SelectionOrbit;

    // --- grid, boundary, and overlay colors ---
    Color Constellation;
    Color Boundary;
    Color EquatorialGrid;
    Color PlanetographicGrid;
    Color PlanetEquator;
    Color GalacticGrid;
    Color EclipticGrid;
    Color HorizonGrid;
    Color Ecliptic;
    Color SelectionCursor;

    // Returns a RendererColors populated with the built-in default sRGB values.
    static RendererColors defaults();

    // Returns a copy of this RendererColors with every color converted from
    // sRGB to linear light, ready for use in a linear-light rendering pipeline.
    RendererColors linearize() const;
};

} // namespace celestia::engine
