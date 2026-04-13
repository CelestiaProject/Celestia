// rendercolors.cpp
//
// Copyright (C) 2001-2025, the Celestia Development Team
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include "rendercolors.h"

namespace celestia::engine
{

// All colors are authored as sRGB values.
RendererColors RendererColors::defaults()
{
    return RendererColors
    {
        // label colors
        /* StarLabel               */ Color(0.471f, 0.356f, 0.682f),
        /* PlanetLabel             */ Color(0.407f, 0.333f, 0.964f),
        /* DwarfPlanetLabel        */ Color(0.557f, 0.235f, 0.576f),
        /* MoonLabel               */ Color(0.231f, 0.733f, 0.792f),
        /* MinorMoonLabel          */ Color(0.231f, 0.733f, 0.792f),
        /* AsteroidLabel           */ Color(0.596f, 0.305f, 0.164f),
        /* CometLabel              */ Color(0.768f, 0.607f, 0.227f),
        /* SpacecraftLabel         */ Color(0.93f,  0.93f,  0.93f ),
        /* LocationLabel           */ Color(0.24f,  0.89f,  0.43f ),
        /* GalaxyLabel             */ Color(0.0f,   0.45f,  0.5f  ),
        /* GlobularLabel           */ Color(0.8f,   0.45f,  0.5f  ),
        /* NebulaLabel             */ Color(0.541f, 0.764f, 0.278f),
        /* OpenClusterLabel        */ Color(0.239f, 0.572f, 0.396f),
        /* ConstellationLabel      */ Color(0.225f, 0.301f, 0.36f ),
        /* EquatorialGridLabel     */ Color(0.64f,  0.72f,  0.88f ),
        /* PlanetographicGridLabel */ Color(0.8f,   0.8f,   0.8f  ),
        /* GalacticGridLabel       */ Color(0.88f,  0.72f,  0.64f ),
        /* EclipticGridLabel       */ Color(0.72f,  0.64f,  0.88f ),
        /* HorizonGridLabel        */ Color(0.72f,  0.72f,  0.72f ),

        // orbit colors
        /* StarOrbit               */ Color(0.5f,   0.5f,   0.8f  ),
        /* PlanetOrbit             */ Color(0.3f,   0.323f, 0.833f),
        /* DwarfPlanetOrbit        */ Color(0.557f, 0.235f, 0.576f),
        /* MoonOrbit               */ Color(0.08f,  0.407f, 0.392f),
        /* MinorMoonOrbit          */ Color(0.08f,  0.407f, 0.392f),
        /* AsteroidOrbit           */ Color(0.58f,  0.152f, 0.08f ),
        /* CometOrbit              */ Color(0.639f, 0.487f, 0.168f),
        /* SpacecraftOrbit         */ Color(0.4f,   0.4f,   0.4f  ),
        /* SelectionOrbit          */ Color(1.0f,   0.0f,   0.0f  ),

        // grid, boundary, and overlay colors
        /* Constellation           */ Color(0.0f,   0.24f,  0.36f ),
        /* Boundary                */ Color(0.24f,  0.10f,  0.12f ),
        /* EquatorialGrid          */ Color(0.28f,  0.28f,  0.38f ),
        /* PlanetographicGrid      */ Color(0.8f,   0.8f,   0.8f  ),
        /* PlanetEquator           */ Color(0.5f,   1.0f,   1.0f  ),
        /* GalacticGrid            */ Color(0.38f,  0.38f,  0.28f ),
        /* EclipticGrid            */ Color(0.38f,  0.28f,  0.38f ),
        /* HorizonGrid             */ Color(0.38f,  0.38f,  0.38f ),
        /* Ecliptic                */ Color(0.5f,   0.1f,   0.1f  ),
        /* SelectionCursor         */ Color(1.0f,   0.0f,   0.0f  ),
    };
}

RendererColors RendererColors::linearize() const
{
    return RendererColors
    {
        // label colors
        /* StarLabel               */ StarLabel.linearize(),
        /* PlanetLabel             */ PlanetLabel.linearize(),
        /* DwarfPlanetLabel        */ DwarfPlanetLabel.linearize(),
        /* MoonLabel               */ MoonLabel.linearize(),
        /* MinorMoonLabel          */ MinorMoonLabel.linearize(),
        /* AsteroidLabel           */ AsteroidLabel.linearize(),
        /* CometLabel              */ CometLabel.linearize(),
        /* SpacecraftLabel         */ SpacecraftLabel.linearize(),
        /* LocationLabel           */ LocationLabel.linearize(),
        /* GalaxyLabel             */ GalaxyLabel.linearize(),
        /* GlobularLabel           */ GlobularLabel.linearize(),
        /* NebulaLabel             */ NebulaLabel.linearize(),
        /* OpenClusterLabel        */ OpenClusterLabel.linearize(),
        /* ConstellationLabel      */ ConstellationLabel.linearize(),
        /* EquatorialGridLabel     */ EquatorialGridLabel.linearize(),
        /* PlanetographicGridLabel */ PlanetographicGridLabel.linearize(),
        /* GalacticGridLabel       */ GalacticGridLabel.linearize(),
        /* EclipticGridLabel       */ EclipticGridLabel.linearize(),
        /* HorizonGridLabel        */ HorizonGridLabel.linearize(),

        // orbit colors
        /* StarOrbit               */ StarOrbit.linearize(),
        /* PlanetOrbit             */ PlanetOrbit.linearize(),
        /* DwarfPlanetOrbit        */ DwarfPlanetOrbit.linearize(),
        /* MoonOrbit               */ MoonOrbit.linearize(),
        /* MinorMoonOrbit          */ MinorMoonOrbit.linearize(),
        /* AsteroidOrbit           */ AsteroidOrbit.linearize(),
        /* CometOrbit              */ CometOrbit.linearize(),
        /* SpacecraftOrbit         */ SpacecraftOrbit.linearize(),
        /* SelectionOrbit          */ SelectionOrbit.linearize(),

        // grid, boundary, and overlay colors
        /* Constellation           */ Constellation.linearize(),
        /* Boundary                */ Boundary.linearize(),
        /* EquatorialGrid          */ EquatorialGrid.linearize(),
        /* PlanetographicGrid      */ PlanetographicGrid.linearize(),
        /* PlanetEquator           */ PlanetEquator.linearize(),
        /* GalacticGrid            */ GalacticGrid.linearize(),
        /* EclipticGrid            */ EclipticGrid.linearize(),
        /* HorizonGrid             */ HorizonGrid.linearize(),
        /* Ecliptic                */ Ecliptic.linearize(),
        /* SelectionCursor         */ SelectionCursor.linearize(),
    };
}

} // namespace celestia::engine
