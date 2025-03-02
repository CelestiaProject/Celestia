#include "scriptmaps.h"

#include <celengine/body.h>
#include <celengine/location.h>
#include <celengine/render.h>
#include <celestia/celestiacore.h>
#include <celestia/hud.h>

using namespace std::string_view_literals;

namespace celestia::scripts
{

void initRenderFlagMap(ScriptMap<RenderFlags>& RenderFlagMap)
{
    RenderFlagMap["orbits"sv]              = RenderFlags::ShowOrbits;
    RenderFlagMap["fadingorbits"sv]        = RenderFlags::ShowFadingOrbits;
    RenderFlagMap["cloudmaps"sv]           = RenderFlags::ShowCloudMaps;
    RenderFlagMap["constellations"sv]      = RenderFlags::ShowDiagrams;
    RenderFlagMap["galaxies"sv]            = RenderFlags::ShowGalaxies;
    RenderFlagMap["globulars"sv]           = RenderFlags::ShowGlobulars;
    RenderFlagMap["planets"sv]             = RenderFlags::ShowPlanets;
    RenderFlagMap["dwarfplanets"sv]        = RenderFlags::ShowDwarfPlanets;
    RenderFlagMap["moons"sv]               = RenderFlags::ShowMoons;
    RenderFlagMap["minormoons"sv]          = RenderFlags::ShowMinorMoons;
    RenderFlagMap["asteroids"sv]           = RenderFlags::ShowAsteroids;
    RenderFlagMap["comets"sv]              = RenderFlags::ShowComets;
    RenderFlagMap["spacecraft"sv]          = RenderFlags::ShowSpacecrafts;
    RenderFlagMap["stars"sv]               = RenderFlags::ShowStars;
    RenderFlagMap["nightmaps"sv]           = RenderFlags::ShowNightMaps;
    RenderFlagMap["eclipseshadows"sv]      = RenderFlags::ShowEclipseShadows;
    RenderFlagMap["planetrings"sv]         = RenderFlags::ShowPlanetRings;
    RenderFlagMap["ringshadows"sv]         = RenderFlags::ShowRingShadows;
    RenderFlagMap["comettails"sv]          = RenderFlags::ShowCometTails;
    RenderFlagMap["boundaries"sv]          = RenderFlags::ShowBoundaries;
    RenderFlagMap["markers"sv]             = RenderFlags::ShowMarkers;
    RenderFlagMap["automag"sv]             = RenderFlags::ShowAutoMag;
    RenderFlagMap["atmospheres"sv]         = RenderFlags::ShowAtmospheres;
    RenderFlagMap["grid"sv]                = RenderFlags::ShowCelestialSphere;
    RenderFlagMap["equatorialgrid"sv]      = RenderFlags::ShowCelestialSphere;
    RenderFlagMap["galacticgrid"sv]        = RenderFlags::ShowGalacticGrid;
    RenderFlagMap["eclipticgrid"sv]        = RenderFlags::ShowEclipticGrid;
    RenderFlagMap["horizontalgrid"sv]      = RenderFlags::ShowHorizonGrid;
    RenderFlagMap["smoothlines"sv]         = RenderFlags::ShowSmoothLines;
    RenderFlagMap["partialtrajectories"sv] = RenderFlags::ShowPartialTrajectories;
    RenderFlagMap["nebulae"sv]             = RenderFlags::ShowNebulae;
    RenderFlagMap["openclusters"sv]        = RenderFlags::ShowOpenClusters;
    RenderFlagMap["cloudshadows"sv]        = RenderFlags::ShowCloudShadows;
    RenderFlagMap["ecliptic"sv]            = RenderFlags::ShowEcliptic;
}

void initLabelFlagMap(ScriptMap<RenderLabels>& LabelFlagMap)
{
    LabelFlagMap["planets"sv]              = RenderLabels::PlanetLabels;
    LabelFlagMap["dwarfplanets"sv]         = RenderLabels::DwarfPlanetLabels;
    LabelFlagMap["moons"sv]                = RenderLabels::MoonLabels;
    LabelFlagMap["minormoons"sv]           = RenderLabels::MinorMoonLabels;
    LabelFlagMap["spacecraft"sv]           = RenderLabels::SpacecraftLabels;
    LabelFlagMap["asteroids"sv]            = RenderLabels::AsteroidLabels;
    LabelFlagMap["comets"sv]               = RenderLabels::CometLabels;
    LabelFlagMap["constellations"sv]       = RenderLabels::ConstellationLabels;
    LabelFlagMap["stars"sv]                = RenderLabels::StarLabels;
    LabelFlagMap["galaxies"sv]             = RenderLabels::GalaxyLabels;
    LabelFlagMap["globulars"sv]            = RenderLabels::GlobularLabels;
    LabelFlagMap["locations"sv]            = RenderLabels::LocationLabels;
    LabelFlagMap["nebulae"sv]              = RenderLabels::NebulaLabels;
    LabelFlagMap["openclusters"sv]         = RenderLabels::OpenClusterLabels;
    LabelFlagMap["i18nconstellations"sv]   = RenderLabels::I18nConstellationLabels;
}

void initBodyTypeMap(ScriptMap<BodyClassification>& BodyTypeMap)
{
    BodyTypeMap["Planet"sv]                = BodyClassification::Planet;
    BodyTypeMap["DwarfPlanet"sv]           = BodyClassification::DwarfPlanet;
    BodyTypeMap["Moon"sv]                  = BodyClassification::Moon;
    BodyTypeMap["MinorMoon"sv]             = BodyClassification::MinorMoon;
    BodyTypeMap["Asteroid"sv]              = BodyClassification::Asteroid;
    BodyTypeMap["Comet"sv]                 = BodyClassification::Comet;
    BodyTypeMap["Spacecraft"sv]            = BodyClassification::Spacecraft;
    BodyTypeMap["Invisible"sv]             = BodyClassification::Invisible;
    BodyTypeMap["Star"sv]                  = BodyClassification::Stellar;
    BodyTypeMap["Unknown"sv]               = BodyClassification::Unknown;
}

void initLocationFlagMap(ScriptMap<std::uint64_t>& LocationFlagMap)
{
    LocationFlagMap["city"sv]              = Location::City;
    LocationFlagMap["observatory"sv]       = Location::Observatory;
    LocationFlagMap["landingsite"sv]       = Location::LandingSite;
    LocationFlagMap["crater"sv]            = Location::Crater;
    LocationFlagMap["vallis"sv]            = Location::Vallis;
    LocationFlagMap["mons"sv]              = Location::Mons;
    LocationFlagMap["planum"sv]            = Location::Planum;
    LocationFlagMap["chasma"sv]            = Location::Chasma;
    LocationFlagMap["patera"sv]            = Location::Patera;
    LocationFlagMap["mare"sv]              = Location::Mare;
    LocationFlagMap["rupes"sv]             = Location::Rupes;
    LocationFlagMap["tessera"sv]           = Location::Tessera;
    LocationFlagMap["regio"sv]             = Location::Regio;
    LocationFlagMap["chaos"sv]             = Location::Chaos;
    LocationFlagMap["terra"sv]             = Location::Terra;
    LocationFlagMap["volcano"sv]           = Location::EruptiveCenter;
    LocationFlagMap["astrum"sv]            = Location::Astrum;
    LocationFlagMap["corona"sv]            = Location::Corona;
    LocationFlagMap["dorsum"sv]            = Location::Dorsum;
    LocationFlagMap["fossa"sv]             = Location::Fossa;
    LocationFlagMap["catena"sv]            = Location::Catena;
    LocationFlagMap["mensa"sv]             = Location::Mensa;
    LocationFlagMap["rima"sv]              = Location::Rima;
    LocationFlagMap["undae"sv]             = Location::Undae;
    LocationFlagMap["tholus"sv]            = Location::Tholus;
    LocationFlagMap["reticulum"sv]         = Location::Reticulum;
    LocationFlagMap["planitia"sv]          = Location::Planitia;
    LocationFlagMap["linea"sv]             = Location::Linea;
    LocationFlagMap["fluctus"sv]           = Location::Fluctus;
    LocationFlagMap["farrum"sv]            = Location::Farrum;
    LocationFlagMap["insula"sv]            = Location::Insula;
    LocationFlagMap["albedo"sv]            = Location::Albedo;
    LocationFlagMap["arcus"sv]             = Location::Arcus;
    LocationFlagMap["cavus"sv]             = Location::Cavus;
    LocationFlagMap["colles"sv]            = Location::Colles;
    LocationFlagMap["facula"sv]            = Location::Facula;
    LocationFlagMap["flexus"sv]            = Location::Flexus;
    LocationFlagMap["flumen"sv]            = Location::Flumen;
    LocationFlagMap["fretum"sv]            = Location::Fretum;
    LocationFlagMap["labes"sv]             = Location::Labes;
    LocationFlagMap["labyrinthus"sv]       = Location::Labyrinthus;
    LocationFlagMap["lacuna"sv]            = Location::Lacuna;
    LocationFlagMap["lacus"sv]             = Location::Lacus;
    LocationFlagMap["largeringed"sv]       = Location::LargeRinged;
    LocationFlagMap["lingula"sv]           = Location::Lingula;
    LocationFlagMap["lobus"sv]             = Location::Lobus;
    LocationFlagMap["macula"sv]            = Location::Macula;
    LocationFlagMap["oceanus"sv]           = Location::Oceanus;
    LocationFlagMap["palus"sv]             = Location::Palus;
    LocationFlagMap["plume"sv]             = Location::Plume;
    LocationFlagMap["promontorium"sv]      = Location::Promontorium;
    LocationFlagMap["satellite"sv]         = Location::Satellite;
    LocationFlagMap["scopulus"sv]          = Location::Scopulus;
    LocationFlagMap["serpens"sv]           = Location::Serpens;
    LocationFlagMap["sinus"sv]             = Location::Sinus;
    LocationFlagMap["sulcus"sv]            = Location::Sulcus;
    LocationFlagMap["vastitas"sv]          = Location::Vastitas;
    LocationFlagMap["virga"sv]             = Location::Virga;
    LocationFlagMap["other"sv]             = Location::Other;
    LocationFlagMap["saxum"sv]             = Location::Saxum;
    LocationFlagMap["capital"sv]           = Location::Capital;
    LocationFlagMap["cosmodrome"sv]        = Location::Cosmodrome;
    LocationFlagMap["ring"sv]              = Location::Ring;
    LocationFlagMap["historical"sv]        = Location::Historical;
}

void initOverlayElementMap(ScriptMap<std::uint32_t>& OverlayElementMap)
{
    OverlayElementMap["Time"sv]            = static_cast<std::uint32_t>(HudElements::ShowTime);
    OverlayElementMap["Velocity"sv]        = static_cast<std::uint32_t>(HudElements::ShowVelocity);
    OverlayElementMap["Selection"sv]       = static_cast<std::uint32_t>(HudElements::ShowSelection);
    OverlayElementMap["Frame"sv]           = static_cast<std::uint32_t>(HudElements::ShowFrame);
}

void initOrbitVisibilityMap(ScriptMap<std::uint32_t>& OrbitVisibilityMap)
{
    OrbitVisibilityMap["never"sv]          = Body::NeverVisible;
    OrbitVisibilityMap["normal"sv]         = Body::UseClassVisibility;
    OrbitVisibilityMap["always"sv]         = Body::AlwaysVisible;
}

void initLabelColorMap(ScriptMap<Color*>& LabelColorMap)
{
    LabelColorMap["stars"sv]               = &Renderer::StarLabelColor;
    LabelColorMap["planets"sv]             = &Renderer::PlanetLabelColor;
    LabelColorMap["dwarfplanets"sv]        = &Renderer::DwarfPlanetLabelColor;
    LabelColorMap["moons"sv]               = &Renderer::MoonLabelColor;
    LabelColorMap["minormoons"sv]          = &Renderer::MinorMoonLabelColor;
    LabelColorMap["asteroids"sv]           = &Renderer::AsteroidLabelColor;
    LabelColorMap["comets"sv]              = &Renderer::CometLabelColor;
    LabelColorMap["spacecraft"sv]          = &Renderer::SpacecraftLabelColor;
    LabelColorMap["locations"sv]           = &Renderer::LocationLabelColor;
    LabelColorMap["galaxies"sv]            = &Renderer::GalaxyLabelColor;
    LabelColorMap["globulars"sv]           = &Renderer::GlobularLabelColor;
    LabelColorMap["nebulae"sv]             = &Renderer::NebulaLabelColor;
    LabelColorMap["openclusters"sv]        = &Renderer::OpenClusterLabelColor;
    LabelColorMap["constellations"sv]      = &Renderer::ConstellationLabelColor;
    LabelColorMap["equatorialgrid"sv]      = &Renderer::EquatorialGridLabelColor;
    LabelColorMap["galacticgrid"sv]        = &Renderer::GalacticGridLabelColor;
    LabelColorMap["eclipticgrid"sv]        = &Renderer::EclipticGridLabelColor;
    LabelColorMap["horizontalgrid"sv]      = &Renderer::HorizonGridLabelColor;
    LabelColorMap["planetographicgrid"sv]  = &Renderer::PlanetographicGridLabelColor;

}

void initLineColorMap(ScriptMap<Color*>& LineColorMap)
{
    LineColorMap["starorbits"sv]           = &Renderer::StarOrbitColor;
    LineColorMap["planetorbits"sv]         = &Renderer::PlanetOrbitColor;
    LineColorMap["dwarfplanetorbits"sv]    = &Renderer::DwarfPlanetOrbitColor;
    LineColorMap["moonorbits"sv]           = &Renderer::MoonOrbitColor;
    LineColorMap["minormoonorbits"sv]      = &Renderer::MinorMoonOrbitColor;
    LineColorMap["asteroidorbits"sv]       = &Renderer::AsteroidOrbitColor;
    LineColorMap["cometorbits"sv]          = &Renderer::CometOrbitColor;
    LineColorMap["spacecraftorbits"sv]     = &Renderer::SpacecraftOrbitColor;
    LineColorMap["constellations"sv]       = &Renderer::ConstellationColor;
    LineColorMap["boundaries"sv]           = &Renderer::BoundaryColor;
    LineColorMap["equatorialgrid"sv]       = &Renderer::EquatorialGridColor;
    LineColorMap["galacticgrid"sv]         = &Renderer::GalacticGridColor;
    LineColorMap["eclipticgrid"sv]         = &Renderer::EclipticGridColor;
    LineColorMap["horizontalgrid"sv]       = &Renderer::HorizonGridColor;
    LineColorMap["planetographicgrid"sv]   = &Renderer::PlanetographicGridColor;
    LineColorMap["planetequator"sv]        = &Renderer::PlanetEquatorColor;
    LineColorMap["ecliptic"sv]             = &Renderer::EclipticColor;
    LineColorMap["selectioncursor"sv]      = &Renderer::SelectionCursorColor;
}

ScriptMaps::ScriptMaps()
{
    initRenderFlagMap(RenderFlagMap);
    initLabelFlagMap(LabelFlagMap);
    initBodyTypeMap(BodyTypeMap);
    initLocationFlagMap(LocationFlagMap);
    initOverlayElementMap(OverlayElementMap);
    initOrbitVisibilityMap(OrbitVisibilityMap);
    initLabelColorMap(LabelColorMap);
    initLineColorMap(LineColorMap);
}

} // end namespace celestia::scripts
