#pragma once

#include <cstdint>

#include <celutil/flag.h>

enum class RenderLabels : std::uint32_t
{
    NoLabels            = 0x000,
    StarLabels          = 0x001,
    PlanetLabels        = 0x002,
    MoonLabels          = 0x004,
    ConstellationLabels = 0x008,
    GalaxyLabels        = 0x010,
    AsteroidLabels      = 0x020,
    SpacecraftLabels    = 0x040,
    LocationLabels      = 0x080,
    CometLabels         = 0x100,
    NebulaLabels        = 0x200,
    OpenClusterLabels   = 0x400,
    I18nConstellationLabels = 0x800,
    DwarfPlanetLabels   = 0x1000,
    MinorMoonLabels     = 0x2000,
    GlobularLabels      = 0x4000,
    BodyLabelMask       = PlanetLabels      |
                          DwarfPlanetLabels |
                          MoonLabels        |
                          MinorMoonLabels   |
                          AsteroidLabels    |
                          SpacecraftLabels  |
                          CometLabels,
};

ENUM_CLASS_BITWISE_OPS(RenderLabels);

enum class RenderFlags : std::uint64_t
{
    ShowNothing             = 0x0000000000000000,
    ShowStars               = 0x0000000000000001,
    ShowPlanets             = 0x0000000000000002,
    ShowGalaxies            = 0x0000000000000004,
    ShowDiagrams            = 0x0000000000000008,
    ShowCloudMaps           = 0x0000000000000010,
    ShowOrbits              = 0x0000000000000020,
    ShowCelestialSphere     = 0x0000000000000040,
    ShowNightMaps           = 0x0000000000000080,
    ShowAtmospheres         = 0x0000000000000100,
    ShowSmoothLines         = 0x0000000000000200,
    ShowEclipseShadows      = 0x0000000000000400,
    // the next one is unused in 1.7, kept for compatibility with 1.6
    ShowStarsAsPoints       = 0x0000000000000800,
    ShowRingShadows         = 0x0000000000001000,
    ShowBoundaries          = 0x0000000000002000,
    ShowAutoMag             = 0x0000000000004000,
    ShowCometTails          = 0x0000000000008000,
    ShowMarkers             = 0x0000000000010000,
    ShowPartialTrajectories = 0x0000000000020000,
    ShowNebulae             = 0x0000000000040000,
    ShowOpenClusters        = 0x0000000000080000,
    ShowGlobulars           = 0x0000000000100000,
    ShowCloudShadows        = 0x0000000000200000,
    ShowGalacticGrid        = 0x0000000000400000,
    ShowEclipticGrid        = 0x0000000000800000,
    ShowHorizonGrid         = 0x0000000001000000,
    ShowEcliptic            = 0x0000000002000000,
    // options added in 1.7
    // removed flag         = 0x0000000004000000,
    ShowDwarfPlanets        = 0x0000000008000000,
    ShowMoons               = 0x0000000010000000,
    ShowMinorMoons          = 0x0000000020000000,
    ShowAsteroids           = 0x0000000040000000,
    ShowComets              = 0x0000000080000000,
    ShowSpacecrafts         = 0x0000000100000000,
    ShowFadingOrbits        = 0x0000000200000000,
    ShowPlanetRings         = 0x0000000400000000,
    ShowSolarSystemObjects  = ShowPlanets           |
                              ShowDwarfPlanets      |
                              ShowMoons             |
                              ShowMinorMoons        |
                              ShowAsteroids         |
                              ShowComets            |
                              ShowSpacecrafts,
    ShowDeepSpaceObjects    = ShowGalaxies          |
                              ShowGlobulars         |
                              ShowNebulae           |
                              ShowOpenClusters,
    DefaultRenderFlags      = ShowStars             |
                              ShowSolarSystemObjects|
                              ShowPlanetRings       |
                              ShowDeepSpaceObjects  |
                              ShowCloudMaps         |
                              ShowNightMaps         |
                              ShowAtmospheres       |
                              ShowEclipseShadows    |
                              ShowRingShadows       |
                              ShowCloudShadows      |
                              ShowCometTails        |
                              ShowAutoMag           |
                              ShowPlanetRings       |
                              ShowFadingOrbits      |
                              ShowSmoothLines,
};

ENUM_CLASS_BITWISE_OPS(RenderFlags);

enum class StarStyle : int
{
    FuzzyPointStars  = 0,
    PointStars       = 1,
    ScaledDiscStars  = 2,
    StarStyleCount,
};
