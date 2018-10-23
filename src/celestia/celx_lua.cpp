
#include "celx_vector.h"
#include "celx_rotation.h"
#include "celx_position.h"
#include "celx_frame.h"
#include "celx_phase.h"
#include "celx_object.h"
#include "celx_observer.h"
#include "celx_gl.h"
#include "celx_celestia.h"

#include "celx_lua.h" 

using namespace Eigen;

/**** Implementation of Celx LuaState wrapper ****/

const char* CelxLua::ClassNames[] =
{
    "class_celestia",
    "class_observer",
    "class_object",
    "class_vec3",
    "class_matrix",
    "class_rotation",
    "class_position",
    "class_frame",
    "class_celscript",
    "class_font",
    "class_image",
    "class_texture",
    "class_phase",
};

CelxLua::FlagMap CelxLua::RenderFlagMap;
CelxLua::FlagMap CelxLua::LabelFlagMap;
CelxLua::FlagMap CelxLua::LocationFlagMap;
CelxLua::FlagMap CelxLua::BodyTypeMap;
CelxLua::FlagMap CelxLua::OverlayElementMap;
CelxLua::FlagMap CelxLua::OrbitVisibilityMap;
CelxLua::ColorMap CelxLua::LineColorMap;
CelxLua::ColorMap CelxLua::LabelColorMap;
bool CelxLua::mapsInitialized = false;

CelxLua::CelxLua(lua_State* l) :
m_lua(l)
{
}


CelxLua::~CelxLua()
{
}

// Initialize various maps from named keywords to numeric flags used within celestia:
void CelxLua::initRenderFlagMap()
{
    RenderFlagMap["orbits"] = Renderer::ShowOrbits;
    RenderFlagMap["cloudmaps"] = Renderer::ShowCloudMaps;
    RenderFlagMap["constellations"] = Renderer::ShowDiagrams;
    RenderFlagMap["galaxies"] = Renderer::ShowGalaxies;
    RenderFlagMap["globulars"] = Renderer::ShowGlobulars;
    RenderFlagMap["planets"] = Renderer::ShowPlanets;
    RenderFlagMap["stars"] = Renderer::ShowStars;
    RenderFlagMap["nightmaps"] = Renderer::ShowNightMaps;
    RenderFlagMap["eclipseshadows"] = Renderer::ShowEclipseShadows;
    RenderFlagMap["ringshadows"] = Renderer::ShowRingShadows;
    RenderFlagMap["comettails"] = Renderer::ShowCometTails;
    RenderFlagMap["boundaries"] = Renderer::ShowBoundaries;
    RenderFlagMap["markers"] = Renderer::ShowMarkers;
    RenderFlagMap["automag"] = Renderer::ShowAutoMag;
    RenderFlagMap["atmospheres"] = Renderer::ShowAtmospheres;
    RenderFlagMap["grid"] = Renderer::ShowCelestialSphere;
    RenderFlagMap["equatorialgrid"] = Renderer::ShowCelestialSphere;
    RenderFlagMap["galacticgrid"] = Renderer::ShowGalacticGrid;
    RenderFlagMap["eclipticgrid"] = Renderer::ShowEclipticGrid;
    RenderFlagMap["horizontalgrid"] = Renderer::ShowHorizonGrid;
    RenderFlagMap["smoothlines"] = Renderer::ShowSmoothLines;
    RenderFlagMap["partialtrajectories"] = Renderer::ShowPartialTrajectories;
    RenderFlagMap["nebulae"] = Renderer::ShowNebulae;
    RenderFlagMap["openclusters"] = Renderer::ShowOpenClusters;
    RenderFlagMap["cloudshadows"] = Renderer::ShowCloudShadows;
    RenderFlagMap["ecliptic"] = Renderer::ShowEcliptic;
}

void CelxLua::initLabelFlagMap()
{
    LabelFlagMap["planets"] = Renderer::PlanetLabels;
    LabelFlagMap["dwarfplanets"] = Renderer::DwarfPlanetLabels;
    LabelFlagMap["moons"] = Renderer::MoonLabels;
    LabelFlagMap["minormoons"] = Renderer::MinorMoonLabels;
    LabelFlagMap["spacecraft"] = Renderer::SpacecraftLabels;
    LabelFlagMap["asteroids"] = Renderer::AsteroidLabels;
    LabelFlagMap["comets"] = Renderer::CometLabels;
    LabelFlagMap["constellations"] = Renderer::ConstellationLabels;
    LabelFlagMap["stars"] = Renderer::StarLabels;
    LabelFlagMap["galaxies"] = Renderer::GalaxyLabels;
    LabelFlagMap["globulars"] = Renderer::GlobularLabels;
    LabelFlagMap["locations"] = Renderer::LocationLabels;
    LabelFlagMap["nebulae"] = Renderer::NebulaLabels;
    LabelFlagMap["openclusters"] = Renderer::OpenClusterLabels;
    LabelFlagMap["i18nconstellations"] = Renderer::I18nConstellationLabels;
}

void CelxLua::initBodyTypeMap()
{
    BodyTypeMap["Planet"] = Body::Planet;
    BodyTypeMap["DwarfPlanet"] = Body::DwarfPlanet;
    BodyTypeMap["Moon"] = Body::Moon;
    BodyTypeMap["MinorMoon"] = Body::MinorMoon;
    BodyTypeMap["Asteroid"] = Body::Asteroid;
    BodyTypeMap["Comet"] = Body::Comet;
    BodyTypeMap["Spacecraft"] = Body::Spacecraft;
    BodyTypeMap["Invisible"] = Body::Invisible;
    BodyTypeMap["Star"] = Body::Stellar;
    BodyTypeMap["Unknown"] = Body::Unknown;
}

void CelxLua::initLocationFlagMap()
{
    LocationFlagMap["city"] = Location::City;
    LocationFlagMap["observatory"] = Location::Observatory;
    LocationFlagMap["landingsite"] = Location::LandingSite;
    LocationFlagMap["crater"] = Location::Crater;
    LocationFlagMap["vallis"] = Location::Vallis;
    LocationFlagMap["mons"] = Location::Mons;
    LocationFlagMap["planum"] = Location::Planum;
    LocationFlagMap["chasma"] = Location::Chasma;
    LocationFlagMap["patera"] = Location::Patera;
    LocationFlagMap["mare"] = Location::Mare;
    LocationFlagMap["rupes"] = Location::Rupes;
    LocationFlagMap["tessera"] = Location::Tessera;
    LocationFlagMap["regio"] = Location::Regio;
    LocationFlagMap["chaos"] = Location::Chaos;
    LocationFlagMap["terra"] = Location::Terra;
    LocationFlagMap["volcano"] = Location::EruptiveCenter;
    LocationFlagMap["astrum"] = Location::Astrum;
    LocationFlagMap["corona"] = Location::Corona;
    LocationFlagMap["dorsum"] = Location::Dorsum;
    LocationFlagMap["fossa"] = Location::Fossa;
    LocationFlagMap["catena"] = Location::Catena;
    LocationFlagMap["mensa"] = Location::Mensa;
    LocationFlagMap["rima"] = Location::Rima;
    LocationFlagMap["undae"] = Location::Undae;
    LocationFlagMap["tholus"] = Location::Tholus;
    LocationFlagMap["reticulum"] = Location::Reticulum;
    LocationFlagMap["planitia"] = Location::Planitia;
    LocationFlagMap["linea"] = Location::Linea;
    LocationFlagMap["fluctus"] = Location::Fluctus;
    LocationFlagMap["farrum"] = Location::Farrum;
    LocationFlagMap["insula"] = Location::Insula;
    LocationFlagMap["other"] = Location::Other;
}

void CelxLua::initOverlayElementMap()
{
    OverlayElementMap["Time"] = CelestiaCore::ShowTime;
    OverlayElementMap["Velocity"] = CelestiaCore::ShowVelocity;
    OverlayElementMap["Selection"] = CelestiaCore::ShowSelection;
    OverlayElementMap["Frame"] = CelestiaCore::ShowFrame;
}

void CelxLua::initOrbitVisibilityMap()
{
    OrbitVisibilityMap["never"] = Body::NeverVisible;
    OrbitVisibilityMap["normal"] = Body::UseClassVisibility;
    OrbitVisibilityMap["always"] = Body::AlwaysVisible;
}


void CelxLua::initLabelColorMap()
{
    LabelColorMap["stars"]          = &Renderer::StarLabelColor;
    LabelColorMap["planets"]        = &Renderer::PlanetLabelColor;
    LabelColorMap["dwarfplanets"]   = &Renderer::DwarfPlanetLabelColor;
    LabelColorMap["moons"]          = &Renderer::MoonLabelColor;
    LabelColorMap["minormoons"]     = &Renderer::MinorMoonLabelColor;
    LabelColorMap["asteroids"]      = &Renderer::AsteroidLabelColor;
    LabelColorMap["comets"]         = &Renderer::CometLabelColor;
    LabelColorMap["spacecraft"]     = &Renderer::SpacecraftLabelColor;
    LabelColorMap["locations"]      = &Renderer::LocationLabelColor;
    LabelColorMap["galaxies"]       = &Renderer::GalaxyLabelColor;
    LabelColorMap["globulars"]      = &Renderer::GlobularLabelColor;
    LabelColorMap["nebulae"]        = &Renderer::NebulaLabelColor;
    LabelColorMap["openclusters"]   = &Renderer::OpenClusterLabelColor;
    LabelColorMap["constellations"] = &Renderer::ConstellationLabelColor;
    LabelColorMap["equatorialgrid"] = &Renderer::EquatorialGridLabelColor;
    LabelColorMap["galacticgrid"]   = &Renderer::GalacticGridLabelColor;
    LabelColorMap["eclipticgrid"]   = &Renderer::EclipticGridLabelColor;
    LabelColorMap["horizontalgrid"] = &Renderer::HorizonGridLabelColor;
    LabelColorMap["planetographicgrid"] = &Renderer::PlanetographicGridLabelColor;
}

void CelxLua::initLineColorMap()
{
    LineColorMap["starorbits"]       = &Renderer::StarOrbitColor;
    LineColorMap["planetorbits"]     = &Renderer::PlanetOrbitColor;
    LineColorMap["dwarfplanetorbits"]     = &Renderer::DwarfPlanetOrbitColor;
    LineColorMap["moonorbits"]       = &Renderer::MoonOrbitColor;
    LineColorMap["minormoonorbits"]       = &Renderer::MinorMoonOrbitColor;
    LineColorMap["asteroidorbits"]   = &Renderer::AsteroidOrbitColor;
    LineColorMap["cometorbits"]      = &Renderer::CometOrbitColor;
    LineColorMap["spacecraftorbits"] = &Renderer::SpacecraftOrbitColor;
    LineColorMap["constellations"]   = &Renderer::ConstellationColor;
    LineColorMap["boundaries"]       = &Renderer::BoundaryColor;
    LineColorMap["equatorialgrid"]   = &Renderer::EquatorialGridColor;
    LineColorMap["galacticgrid"]     = &Renderer::GalacticGridColor;
    LineColorMap["eclipticgrid"]     = &Renderer::EclipticGridColor;
    LineColorMap["horizontalgrid"]   = &Renderer::HorizonGridColor;
    LineColorMap["planetographicgrid"]   = &Renderer::PlanetographicGridColor;
    LineColorMap["planetequator"]    = &Renderer::PlanetEquatorColor;
    LineColorMap["ecliptic"]         = &Renderer::EclipticColor;
    LineColorMap["selectioncursor"]  = &Renderer::SelectionCursorColor;
}


void CelxLua::initMaps()
{
    if (!mapsInitialized)
    {
        initRenderFlagMap();
        initLabelFlagMap();
        initBodyTypeMap();
        initLocationFlagMap();
        initOverlayElementMap();
        initOrbitVisibilityMap();
        initLabelColorMap();
        initLineColorMap();
    }
    mapsInitialized = true;
}

bool CelxLua::isType(int index, int type) const
{
    return Celx_istype(m_lua, index, type);
}


void CelxLua::setClass(int id)
{
    Celx_SetClass(m_lua, id);
}


// Push a class name onto the Lua stack
void CelxLua::pushClassName(int id)
{
    lua_pushlstring(m_lua, ClassNames[id], strlen(ClassNames[id]));
}


void* CelxLua::checkUserData(int index, int id)
{
    return Celx_CheckUserData(m_lua, index, id);
}


void CelxLua::doError(const char* errorMessage)
{
    Celx_DoError(m_lua, errorMessage);
}


void CelxLua::checkArgs(int minArgs, int maxArgs, const char* errorMessage)
{
    Celx_CheckArgs(m_lua, minArgs, maxArgs, errorMessage);
}


void CelxLua::createClassMetatable(int id)
{
    Celx_CreateClassMetatable(m_lua, id);
}


void CelxLua::registerMethod(const char* name, lua_CFunction fn)
{
    Celx_RegisterMethod(m_lua, name, fn);
}


void CelxLua::registerValue(const char* name, float n)
{
    lua_pushstring(m_lua, name);
    lua_pushnumber(m_lua, n);
    lua_settable(m_lua, -3);
}


// Add a field to the table on top of the stack
void CelxLua::setTable(const char* field, lua_Number value)
{
    lua_pushstring(m_lua, field);
    lua_pushnumber(m_lua, value);
    lua_settable(m_lua, -3);
}

void CelxLua::setTable(const char* field, const char* value)
{
    lua_pushstring(m_lua, field);
    lua_pushstring(m_lua, value);
    lua_settable(m_lua, -3);
}


lua_Number CelxLua::safeGetNumber(int index,
                                  FatalErrors fatalErrors,
                                  const char* errorMessage,
                                  lua_Number defaultValue)
{
    return Celx_SafeGetNumber(m_lua, index, fatalErrors, errorMessage, defaultValue);
}


const char* CelxLua::safeGetString(int index,
                                   FatalErrors fatalErrors,
                                   const char* errorMessage)
{
    return Celx_SafeGetString(m_lua, index, fatalErrors, errorMessage);
}


bool CelxLua::safeGetBoolean(int index,
                             FatalErrors fatalErrors,
                             const char* errorMessage,
                             bool defaultValue)
{
    return Celx_SafeGetBoolean(m_lua, index, fatalErrors, errorMessage, defaultValue);
}


void CelxLua::newVector(const Vec3d& v)
{
    vector_new(m_lua, v);
}


void CelxLua::newVector(const Vector3d& v)
{
    vector_new(m_lua, fromEigen(v));
}


void CelxLua::newPosition(const UniversalCoord& uc)
{
    position_new(m_lua, uc);
}


void CelxLua::newRotation(const Quatd& q)
{
    rotation_new(m_lua, q);
}

void CelxLua::newObject(const Selection& sel)
{
    object_new(m_lua, sel);
}

void CelxLua::newFrame(const ObserverFrame& f)
{
    frame_new(m_lua, f);
}

void CelxLua::newPhase(const TimelinePhase& phase)
{
    phase_new(m_lua, phase);
}

Vec3d* CelxLua::toVector(int n)
{
    return to_vector(m_lua, n);
}

Quatd* CelxLua::toRotation(int n)
{
    return to_rotation(m_lua, n);
}

UniversalCoord* CelxLua::toPosition(int n)
{
    return to_position(m_lua, n);
}

Selection* CelxLua::toObject(int n)
{
    return to_object(m_lua, n);
}

ObserverFrame* CelxLua::toFrame(int n)
{
    return to_frame(m_lua, n);
}

void CelxLua::push(const CelxValue& v1)
{
    v1.push(m_lua);
}


void CelxLua::push(const CelxValue& v1, const CelxValue& v2)
{
    v1.push(m_lua);
    v2.push(m_lua);
}


CelestiaCore* CelxLua::appCore(FatalErrors fatalErrors)
{
    push("celestia-appcore");
    lua_gettable(m_lua, LUA_REGISTRYINDEX);

    if (!lua_islightuserdata(m_lua, -1))
    {
        if (fatalErrors == NoErrors)
        {
            return NULL;
        }
        else
        {
            lua_pushstring(m_lua, "internal error: invalid appCore");
            lua_error(m_lua);
        }
    }

    CelestiaCore* appCore = static_cast<CelestiaCore*>(lua_touserdata(m_lua, -1));
    lua_pop(m_lua, 1);

    return appCore;
}


// Get a pointer to the LuaState-object from the registry:
LuaState* CelxLua::getLuaStateObject()
{
    int stackSize = lua_gettop(m_lua);
    lua_pushstring(m_lua, "celestia-luastate");
    lua_gettable(m_lua, LUA_REGISTRYINDEX);

    if (!lua_islightuserdata(m_lua, -1))
    {
        Celx_DoError(m_lua, "Internal Error: Invalid table entry for LuaState-pointer");
    }
    LuaState* luastate_ptr = static_cast<LuaState*>(lua_touserdata(m_lua, -1));
    if (luastate_ptr == NULL)
    {
        Celx_DoError(m_lua, "Internal Error: Invalid LuaState-pointer");
    }
    lua_settop(m_lua, stackSize);
    return luastate_ptr;
}
