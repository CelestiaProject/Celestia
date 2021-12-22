#include "scriptmaps.h"
#include <celestia/celestiacore.h>
#include <celengine/render.h>

namespace celestia::scripts
{

void initRenderFlagMap(FlagMap64 &RenderFlagMap)
{
    RenderFlagMap["orbits"]              = Renderer::ShowOrbits;
    RenderFlagMap["fadingorbits"]        = Renderer::ShowFadingOrbits;
    RenderFlagMap["cloudmaps"]           = Renderer::ShowCloudMaps;
    RenderFlagMap["constellations"]      = Renderer::ShowDiagrams;
    RenderFlagMap["galaxies"]            = Renderer::ShowGalaxies;
    RenderFlagMap["globulars"]           = Renderer::ShowGlobulars;
    RenderFlagMap["planets"]             = Renderer::ShowPlanets;
    RenderFlagMap["dwarfplanets"]        = Renderer::ShowDwarfPlanets;
    RenderFlagMap["moons"]               = Renderer::ShowMoons;
    RenderFlagMap["minormoons"]          = Renderer::ShowMinorMoons;
    RenderFlagMap["asteroids"]           = Renderer::ShowAsteroids;
    RenderFlagMap["comets"]              = Renderer::ShowComets;
    RenderFlagMap["spasecrafts"]         = Renderer::ShowSpacecrafts;
    RenderFlagMap["stars"]               = Renderer::ShowStars;
    RenderFlagMap["nightmaps"]           = Renderer::ShowNightMaps;
    RenderFlagMap["eclipseshadows"]      = Renderer::ShowEclipseShadows;
    RenderFlagMap["planetrings"]         = Renderer::ShowPlanetRings;
    RenderFlagMap["ringshadows"]         = Renderer::ShowRingShadows;
    RenderFlagMap["comettails"]          = Renderer::ShowCometTails;
    RenderFlagMap["boundaries"]          = Renderer::ShowBoundaries;
    RenderFlagMap["markers"]             = Renderer::ShowMarkers;
    RenderFlagMap["automag"]             = Renderer::ShowAutoMag;
    RenderFlagMap["atmospheres"]         = Renderer::ShowAtmospheres;
    RenderFlagMap["grid"]                = Renderer::ShowCelestialSphere;
    RenderFlagMap["equatorialgrid"]      = Renderer::ShowCelestialSphere;
    RenderFlagMap["galacticgrid"]        = Renderer::ShowGalacticGrid;
    RenderFlagMap["eclipticgrid"]        = Renderer::ShowEclipticGrid;
    RenderFlagMap["horizontalgrid"]      = Renderer::ShowHorizonGrid;
    RenderFlagMap["smoothlines"]         = Renderer::ShowSmoothLines;
    RenderFlagMap["partialtrajectories"] = Renderer::ShowPartialTrajectories;
    RenderFlagMap["nebulae"]             = Renderer::ShowNebulae;
    RenderFlagMap["openclusters"]        = Renderer::ShowOpenClusters;
    RenderFlagMap["cloudshadows"]        = Renderer::ShowCloudShadows;
    RenderFlagMap["ecliptic"]            = Renderer::ShowEcliptic;
}

void initLabelFlagMap(FlagMap &LabelFlagMap)
{
    LabelFlagMap["planets"]              = Renderer::PlanetLabels;
    LabelFlagMap["dwarfplanets"]         = Renderer::DwarfPlanetLabels;
    LabelFlagMap["moons"]                = Renderer::MoonLabels;
    LabelFlagMap["minormoons"]           = Renderer::MinorMoonLabels;
    LabelFlagMap["spacecraft"]           = Renderer::SpacecraftLabels;
    LabelFlagMap["asteroids"]            = Renderer::AsteroidLabels;
    LabelFlagMap["comets"]               = Renderer::CometLabels;
    LabelFlagMap["constellations"]       = Renderer::ConstellationLabels;
    LabelFlagMap["stars"]                = Renderer::StarLabels;
    LabelFlagMap["galaxies"]             = Renderer::GalaxyLabels;
    LabelFlagMap["globulars"]            = Renderer::GlobularLabels;
    LabelFlagMap["locations"]            = Renderer::LocationLabels;
    LabelFlagMap["nebulae"]              = Renderer::NebulaLabels;
    LabelFlagMap["openclusters"]         = Renderer::OpenClusterLabels;
    LabelFlagMap["i18nconstellations"]   = Renderer::I18nConstellationLabels;
}

void initBodyTypeMap(FlagMap &BodyTypeMap)
{
    BodyTypeMap["Planet"]                = Body::Planet;
    BodyTypeMap["DwarfPlanet"]           = Body::DwarfPlanet;
    BodyTypeMap["Moon"]                  = Body::Moon;
    BodyTypeMap["MinorMoon"]             = Body::MinorMoon;
    BodyTypeMap["Asteroid"]              = Body::Asteroid;
    BodyTypeMap["Comet"]                 = Body::Comet;
    BodyTypeMap["Spacecraft"]            = Body::Spacecraft;
    BodyTypeMap["Invisible"]             = Body::Invisible;
    BodyTypeMap["Star"]                  = Body::Stellar;
    BodyTypeMap["Unknown"]               = Body::Unknown;
}

void initLocationFlagMap(FlagMap64 &LocationFlagMap)
{
    LocationFlagMap["city"]              = Location::City;
    LocationFlagMap["observatory"]       = Location::Observatory;
    LocationFlagMap["landingsite"]       = Location::LandingSite;
    LocationFlagMap["crater"]            = Location::Crater;
    LocationFlagMap["vallis"]            = Location::Vallis;
    LocationFlagMap["mons"]              = Location::Mons;
    LocationFlagMap["planum"]            = Location::Planum;
    LocationFlagMap["chasma"]            = Location::Chasma;
    LocationFlagMap["patera"]            = Location::Patera;
    LocationFlagMap["mare"]              = Location::Mare;
    LocationFlagMap["rupes"]             = Location::Rupes;
    LocationFlagMap["tessera"]           = Location::Tessera;
    LocationFlagMap["regio"]             = Location::Regio;
    LocationFlagMap["chaos"]             = Location::Chaos;
    LocationFlagMap["terra"]             = Location::Terra;
    LocationFlagMap["volcano"]           = Location::EruptiveCenter;
    LocationFlagMap["astrum"]            = Location::Astrum;
    LocationFlagMap["corona"]            = Location::Corona;
    LocationFlagMap["dorsum"]            = Location::Dorsum;
    LocationFlagMap["fossa"]             = Location::Fossa;
    LocationFlagMap["catena"]            = Location::Catena;
    LocationFlagMap["mensa"]             = Location::Mensa;
    LocationFlagMap["rima"]              = Location::Rima;
    LocationFlagMap["undae"]             = Location::Undae;
    LocationFlagMap["tholus"]            = Location::Tholus;
    LocationFlagMap["reticulum"]         = Location::Reticulum;
    LocationFlagMap["planitia"]          = Location::Planitia;
    LocationFlagMap["linea"]             = Location::Linea;
    LocationFlagMap["fluctus"]           = Location::Fluctus;
    LocationFlagMap["farrum"]            = Location::Farrum;
    LocationFlagMap["insula"]            = Location::Insula;
    LocationFlagMap["albedo"]            = Location::Albedo;
    LocationFlagMap["arcus"]             = Location::Arcus;
    LocationFlagMap["cavus"]             = Location::Cavus;
    LocationFlagMap["colles"]            = Location::Colles;
    LocationFlagMap["facula"]            = Location::Facula;
    LocationFlagMap["flexus"]            = Location::Flexus;
    LocationFlagMap["flumen"]            = Location::Flumen;
    LocationFlagMap["fretum"]            = Location::Fretum;
    LocationFlagMap["labes"]             = Location::Labes;
    LocationFlagMap["labyrinthus"]       = Location::Labyrinthus;
    LocationFlagMap["lacuna"]            = Location::Lacuna;
    LocationFlagMap["lacus"]             = Location::Lacus;
    LocationFlagMap["largeringed"]       = Location::LargeRinged;
    LocationFlagMap["lenticula"]         = Location::Lenticula;
    LocationFlagMap["lingula"]           = Location::Lingula;
    LocationFlagMap["macula"]            = Location::Macula;
    LocationFlagMap["oceanus"]           = Location::Oceanus;
    LocationFlagMap["palus"]             = Location::Palus;
    LocationFlagMap["plume"]             = Location::Plume;
    LocationFlagMap["promontorium"]      = Location::Promontorium;
    LocationFlagMap["satellite"]         = Location::Satellite;
    LocationFlagMap["scopulus"]          = Location::Scopulus;
    LocationFlagMap["serpens"]           = Location::Serpens;
    LocationFlagMap["sinus"]             = Location::Sinus;
    LocationFlagMap["sulcus"]            = Location::Sulcus;
    LocationFlagMap["vastitas"]          = Location::Vastitas;
    LocationFlagMap["virga"]             = Location::Virga;
    LocationFlagMap["other"]             = Location::Other;
    LocationFlagMap["saxum"]             = Location::Saxum;
    LocationFlagMap["capital"]           = Location::Capital;
    LocationFlagMap["cosmodrome"]        = Location::Cosmodrome;
    LocationFlagMap["ring"]              = Location::Ring;
    LocationFlagMap["historical"]        = Location::Historical;
}

void initOverlayElementMap(FlagMap &OverlayElementMap)
{
    OverlayElementMap["Time"]            = CelestiaCore::ShowTime;
    OverlayElementMap["Velocity"]        = CelestiaCore::ShowVelocity;
    OverlayElementMap["Selection"]       = CelestiaCore::ShowSelection;
    OverlayElementMap["Frame"]           = CelestiaCore::ShowFrame;
}

void initOrbitVisibilityMap(FlagMap &OrbitVisibilityMap)
{
    OrbitVisibilityMap["never"]          = Body::NeverVisible;
    OrbitVisibilityMap["normal"]         = Body::UseClassVisibility;
    OrbitVisibilityMap["always"]         = Body::AlwaysVisible;
}

void initLabelColorMap(ColorMap &LabelColorMap)
{
    LabelColorMap["stars"]               = &Renderer::StarLabelColor;
    LabelColorMap["planets"]             = &Renderer::PlanetLabelColor;
    LabelColorMap["dwarfplanets"]        = &Renderer::DwarfPlanetLabelColor;
    LabelColorMap["moons"]               = &Renderer::MoonLabelColor;
    LabelColorMap["minormoons"]          = &Renderer::MinorMoonLabelColor;
    LabelColorMap["asteroids"]           = &Renderer::AsteroidLabelColor;
    LabelColorMap["comets"]              = &Renderer::CometLabelColor;
    LabelColorMap["spacecraft"]          = &Renderer::SpacecraftLabelColor;
    LabelColorMap["locations"]           = &Renderer::LocationLabelColor;
    LabelColorMap["galaxies"]            = &Renderer::GalaxyLabelColor;
    LabelColorMap["globulars"]           = &Renderer::GlobularLabelColor;
    LabelColorMap["nebulae"]             = &Renderer::NebulaLabelColor;
    LabelColorMap["openclusters"]        = &Renderer::OpenClusterLabelColor;
    LabelColorMap["constellations"]      = &Renderer::ConstellationLabelColor;
    LabelColorMap["equatorialgrid"]      = &Renderer::EquatorialGridLabelColor;
    LabelColorMap["galacticgrid"]        = &Renderer::GalacticGridLabelColor;
    LabelColorMap["eclipticgrid"]        = &Renderer::EclipticGridLabelColor;
    LabelColorMap["horizontalgrid"]      = &Renderer::HorizonGridLabelColor;
    LabelColorMap["planetographicgrid"]  = &Renderer::PlanetographicGridLabelColor;

}

void initLineColorMap(ColorMap &LineColorMap)
{
    LineColorMap["starorbits"]           = &Renderer::StarOrbitColor;
    LineColorMap["planetorbits"]         = &Renderer::PlanetOrbitColor;
    LineColorMap["dwarfplanetorbits"]    = &Renderer::DwarfPlanetOrbitColor;
    LineColorMap["moonorbits"]           = &Renderer::MoonOrbitColor;
    LineColorMap["minormoonorbits"]      = &Renderer::MinorMoonOrbitColor;
    LineColorMap["asteroidorbits"]       = &Renderer::AsteroidOrbitColor;
    LineColorMap["cometorbits"]          = &Renderer::CometOrbitColor;
    LineColorMap["spacecraftorbits"]     = &Renderer::SpacecraftOrbitColor;
    LineColorMap["constellations"]       = &Renderer::ConstellationColor;
    LineColorMap["boundaries"]           = &Renderer::BoundaryColor;
    LineColorMap["equatorialgrid"]       = &Renderer::EquatorialGridColor;
    LineColorMap["galacticgrid"]         = &Renderer::GalacticGridColor;
    LineColorMap["eclipticgrid"]         = &Renderer::EclipticGridColor;
    LineColorMap["horizontalgrid"]       = &Renderer::HorizonGridColor;
    LineColorMap["planetographicgrid"]   = &Renderer::PlanetographicGridColor;
    LineColorMap["planetequator"]        = &Renderer::PlanetEquatorColor;
    LineColorMap["ecliptic"]             = &Renderer::EclipticColor;
    LineColorMap["selectioncursor"]      = &Renderer::SelectionCursorColor;
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
