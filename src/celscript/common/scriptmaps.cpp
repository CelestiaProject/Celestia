#include <celengine/body.h>
#include <celengine/render.h>
#include <celestia/celestiacore.h>
#include "scriptmaps.h"

using namespace std::string_view_literals;

namespace celestia::scripts
{

void initRenderFlagMap(FlagMap64 &RenderFlagMap)
{
    RenderFlagMap["orbits"sv]              = Renderer::ShowOrbits;
    RenderFlagMap["fadingorbits"sv]        = Renderer::ShowFadingOrbits;
    RenderFlagMap["cloudmaps"sv]           = Renderer::ShowCloudMaps;
    RenderFlagMap["constellations"sv]      = Renderer::ShowDiagrams;
    RenderFlagMap["galaxies"sv]            = Renderer::ShowGalaxies;
    RenderFlagMap["globulars"sv]           = Renderer::ShowGlobulars;
    RenderFlagMap["planets"sv]             = Renderer::ShowPlanets;
    RenderFlagMap["dwarfplanets"sv]        = Renderer::ShowDwarfPlanets;
    RenderFlagMap["moons"sv]               = Renderer::ShowMoons;
    RenderFlagMap["minormoons"sv]          = Renderer::ShowMinorMoons;
    RenderFlagMap["asteroids"sv]           = Renderer::ShowAsteroids;
    RenderFlagMap["comets"sv]              = Renderer::ShowComets;
    RenderFlagMap["spasecrafts"sv]         = Renderer::ShowSpacecrafts;
    RenderFlagMap["stars"sv]               = Renderer::ShowStars;
    RenderFlagMap["nightmaps"sv]           = Renderer::ShowNightMaps;
    RenderFlagMap["eclipseshadows"sv]      = Renderer::ShowEclipseShadows;
    RenderFlagMap["planetrings"sv]         = Renderer::ShowPlanetRings;
    RenderFlagMap["ringshadows"sv]         = Renderer::ShowRingShadows;
    RenderFlagMap["comettails"sv]          = Renderer::ShowCometTails;
    RenderFlagMap["boundaries"sv]          = Renderer::ShowBoundaries;
    RenderFlagMap["markers"sv]             = Renderer::ShowMarkers;
    RenderFlagMap["automag"sv]             = Renderer::ShowAutoMag;
    RenderFlagMap["atmospheres"sv]         = Renderer::ShowAtmospheres;
    RenderFlagMap["grid"sv]                = Renderer::ShowCelestialSphere;
    RenderFlagMap["equatorialgrid"sv]      = Renderer::ShowCelestialSphere;
    RenderFlagMap["galacticgrid"sv]        = Renderer::ShowGalacticGrid;
    RenderFlagMap["eclipticgrid"sv]        = Renderer::ShowEclipticGrid;
    RenderFlagMap["horizontalgrid"sv]      = Renderer::ShowHorizonGrid;
    RenderFlagMap["smoothlines"sv]         = Renderer::ShowSmoothLines;
    RenderFlagMap["partialtrajectories"sv] = Renderer::ShowPartialTrajectories;
    RenderFlagMap["nebulae"sv]             = Renderer::ShowNebulae;
    RenderFlagMap["openclusters"sv]        = Renderer::ShowOpenClusters;
    RenderFlagMap["cloudshadows"sv]        = Renderer::ShowCloudShadows;
    RenderFlagMap["ecliptic"sv]            = Renderer::ShowEcliptic;
}

void initLabelFlagMap(FlagMap &LabelFlagMap)
{
    LabelFlagMap["planets"sv]              = Renderer::PlanetLabels;
    LabelFlagMap["dwarfplanets"sv]         = Renderer::DwarfPlanetLabels;
    LabelFlagMap["moons"sv]                = Renderer::MoonLabels;
    LabelFlagMap["minormoons"sv]           = Renderer::MinorMoonLabels;
    LabelFlagMap["spacecraft"sv]           = Renderer::SpacecraftLabels;
    LabelFlagMap["asteroids"sv]            = Renderer::AsteroidLabels;
    LabelFlagMap["comets"sv]               = Renderer::CometLabels;
    LabelFlagMap["constellations"sv]       = Renderer::ConstellationLabels;
    LabelFlagMap["stars"sv]                = Renderer::StarLabels;
    LabelFlagMap["galaxies"sv]             = Renderer::GalaxyLabels;
    LabelFlagMap["globulars"sv]            = Renderer::GlobularLabels;
    LabelFlagMap["locations"sv]            = Renderer::LocationLabels;
    LabelFlagMap["nebulae"sv]              = Renderer::NebulaLabels;
    LabelFlagMap["openclusters"sv]         = Renderer::OpenClusterLabels;
    LabelFlagMap["i18nconstellations"sv]   = Renderer::I18nConstellationLabels;
}

void initBodyTypeMap(FlagMap &BodyTypeMap)
{
    BodyTypeMap["Planet"sv]                = Body::Planet;
    BodyTypeMap["DwarfPlanet"sv]           = Body::DwarfPlanet;
    BodyTypeMap["Moon"sv]                  = Body::Moon;
    BodyTypeMap["MinorMoon"sv]             = Body::MinorMoon;
    BodyTypeMap["Asteroid"sv]              = Body::Asteroid;
    BodyTypeMap["Comet"sv]                 = Body::Comet;
    BodyTypeMap["Spacecraft"sv]            = Body::Spacecraft;
    BodyTypeMap["Invisible"sv]             = Body::Invisible;
    BodyTypeMap["Star"sv]                  = Body::Stellar;
    BodyTypeMap["Unknown"sv]               = Body::Unknown;
}

void initLocationFlagMap(FlagMap64 &LocationFlagMap)
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
    LocationFlagMap["lenticula"sv]         = Location::Lenticula;
    LocationFlagMap["lingula"sv]           = Location::Lingula;
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

void initOverlayElementMap(FlagMap &OverlayElementMap)
{
    OverlayElementMap["Time"sv]            = CelestiaCore::ShowTime;
    OverlayElementMap["Velocity"sv]        = CelestiaCore::ShowVelocity;
    OverlayElementMap["Selection"sv]       = CelestiaCore::ShowSelection;
    OverlayElementMap["Frame"sv]           = CelestiaCore::ShowFrame;
}

void initOrbitVisibilityMap(FlagMap &OrbitVisibilityMap)
{
    OrbitVisibilityMap["never"sv]          = Body::NeverVisible;
    OrbitVisibilityMap["normal"sv]         = Body::UseClassVisibility;
    OrbitVisibilityMap["always"sv]         = Body::AlwaysVisible;
}

void initLabelColorMap(ColorMap &LabelColorMap)
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

void initLineColorMap(ColorMap &LineColorMap)
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
