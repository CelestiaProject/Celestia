// celx.cpp
//
// Copyright (C) 2003-2008, the Celestia Development Team
// Original version by Chris Laurel <claurel@gmail.com>
//
// Lua script extensions for Celestia.
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include <cassert>
#include <ctime>
#include <map>
#include <celengine/astro.h>
#include <celengine/asterism.h>
#include <celengine/celestia.h>
#include <celengine/cmdparser.h>
#include <celengine/execenv.h>
#include <celengine/execution.h>
#ifdef __CELVEC__
#include <celmath/vecmath.h>
#endif
#include <celengine/timeline.h>
#include <celengine/timelinephase.h>
#include <fmt/printf.h>
#include "imagecapture.h"
#include "url.h"

#include "celx.h"
#include "celx_internal.h"
#include "celx_misc.h"
#include "celx_vector.h"
#include "celx_rotation.h"
#include "celx_position.h"
#include "celx_frame.h"
#include "celx_phase.h"
#include "celx_object.h"
#include "celx_observer.h"
#include "celx_celestia.h"
#include "celx_gl.h"

#ifdef __CELVEC__
#include <celengine/eigenport.h>
#endif


// Older gcc versions used <strstream> instead of <sstream>.
// This has been corrected in GCC 3.2, but name clashing must
// be avoided
#ifdef __GNUC__
#undef min
#undef max
#endif
#include <sstream>
#include <utility>

#include "celx.h"
#include "celestiacore.h"

using namespace Eigen;
using namespace std;

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

CelxLua::FlagMap64 CelxLua::RenderFlagMap;
CelxLua::FlagMap CelxLua::LabelFlagMap;
CelxLua::FlagMap64 CelxLua::LocationFlagMap;
CelxLua::FlagMap CelxLua::BodyTypeMap;
CelxLua::FlagMap CelxLua::OverlayElementMap;
CelxLua::FlagMap CelxLua::OrbitVisibilityMap;
CelxLua::ColorMap CelxLua::LineColorMap;
CelxLua::ColorMap CelxLua::LabelColorMap;
bool CelxLua::mapsInitialized = false;


#define CLASS(i) ClassNames[(i)]

// Maximum timeslice a script may run without
// returning control to celestia
static const double MaxTimeslice = 5.0;

// names of callback-functions in Lua:
const char* KbdCallback = "celestia_keyboard_callback";
const char* CleanupCallback = "celestia_cleanup_callback";

const char* EventHandlers = "celestia_event_handlers";

const char* KeyHandler        = "key";
const char* TickHandler       = "tick";
const char* MouseDownHandler  = "mousedown";
const char* MouseUpHandler    = "mouseup";


// Initialize various maps from named keywords to numeric flags used within celestia:
void CelxLua::initRenderFlagMap()
{
    RenderFlagMap["orbits"]              = Renderer::ShowOrbits;
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
    LocationFlagMap["albedo"] = Location::Albedo;
    LocationFlagMap["arcus"] = Location::Arcus;
    LocationFlagMap["cavus"] = Location::Cavus;
    LocationFlagMap["colles"] = Location::Colles;
    LocationFlagMap["facula"] = Location::Facula;
    LocationFlagMap["flexus"] = Location::Flexus;
    LocationFlagMap["flumen"] = Location::Flumen;
    LocationFlagMap["fretum"] = Location::Fretum;
    LocationFlagMap["labes"] = Location::Labes;
    LocationFlagMap["labyrinthus"] = Location::Labyrinthus;
    LocationFlagMap["lacuna"] = Location::Lacuna;
    LocationFlagMap["lacus"] = Location::Lacus;
    LocationFlagMap["large"] = Location::Large;
    LocationFlagMap["lenticula"] = Location::Lenticula;
    LocationFlagMap["lingula"] = Location::Lingula;
    LocationFlagMap["macula"] = Location::Macula;
    LocationFlagMap["oceanus"] = Location::Oceanus;
    LocationFlagMap["palus"] = Location::Palus;
    LocationFlagMap["plume"] = Location::Plume;
    LocationFlagMap["promontorium"] = Location::Promontorium;
    LocationFlagMap["satellite"] = Location::Satellite;
    LocationFlagMap["scopulus"] = Location::Scopulus;
    LocationFlagMap["serpens"] = Location::Serpens;
    LocationFlagMap["sinus"] = Location::Sinus;
    LocationFlagMap["sulcus"] = Location::Sulcus;
    LocationFlagMap["vastitas"] = Location::Vastitas;
    LocationFlagMap["virga"] = Location::Virga;
    LocationFlagMap["other"] = Location::Other;
    LocationFlagMap["saxum"] = Location::Saxum;
    LocationFlagMap["capital"] = Location::Capital;
    LocationFlagMap["cosmodrome"] = Location::Cosmodrome;
    LocationFlagMap["trench"] = Location::Trench;
    LocationFlagMap["historical"] = Location::Historical;
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
    LineColorMap["dwarfplanetorbits"]= &Renderer::DwarfPlanetOrbitColor;
    LineColorMap["moonorbits"]       = &Renderer::MoonOrbitColor;
    LineColorMap["minormoonorbits"]  = &Renderer::MinorMoonOrbitColor;
    LineColorMap["asteroidorbits"]   = &Renderer::AsteroidOrbitColor;
    LineColorMap["cometorbits"]      = &Renderer::CometOrbitColor;
    LineColorMap["spacecraftorbits"] = &Renderer::SpacecraftOrbitColor;
    LineColorMap["constellations"]   = &Renderer::ConstellationColor;
    LineColorMap["boundaries"]       = &Renderer::BoundaryColor;
    LineColorMap["equatorialgrid"]   = &Renderer::EquatorialGridColor;
    LineColorMap["galacticgrid"]     = &Renderer::GalacticGridColor;
    LineColorMap["eclipticgrid"]     = &Renderer::EclipticGridColor;
    LineColorMap["horizontalgrid"]   = &Renderer::HorizonGridColor;
    LineColorMap["planetographicgrid"] = &Renderer::PlanetographicGridColor;
    LineColorMap["planetequator"]    = &Renderer::PlanetEquatorColor;
    LineColorMap["ecliptic"]         = &Renderer::EclipticColor;
    LineColorMap["selectioncursor"]  = &Renderer::SelectionCursorColor;
}


#if LUA_VER >= 0x050100
// Load a Lua library--in Lua 5.1, the luaopen_* functions cannot be called
// directly. They most be invoked through the Lua state.
static void openLuaLibrary(lua_State* l,
                           const char* name,
                           lua_CFunction func)
{
#if LUA_VER >= 0x050200
    luaL_requiref(l, name, func, 1);
#else
    lua_pushcfunction(l, func);
    lua_pushstring(l, name);
    lua_call(l, 1, 0);
#endif
}
#endif


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


static void getField(lua_State* l, int index, const char* key)
{
    // When we move to Lua 5.1, this will be replaced by:
    // lua_getfield(l, index, key);
    lua_pushstring(l, key);
#ifdef LUA_GLOBALSINDEX
    if (index == LUA_GLOBALSINDEX) {
      lua_gettable(l, index);
      return;
    }
#endif
    if (index != LUA_REGISTRYINDEX)
        lua_gettable(l, index - 1);
    else
        lua_gettable(l, index);
}


// Push a class name onto the Lua stack
void PushClass(lua_State* l, int id)
{
    lua_pushlstring(l, CelxLua::ClassNames[id], strlen(CelxLua::ClassNames[id]));
}

// Set the class (metatable) of the object on top of the stack
void Celx_SetClass(lua_State* l, int id)
{
    PushClass(l, id);
    lua_rawget(l, LUA_REGISTRYINDEX);
    if (lua_type(l, -1) != LUA_TTABLE)
        cout << "Metatable for " << CelxLua::ClassNames[id] << " not found!\n";
    if (lua_setmetatable(l, -2) == 0)
        cout << "Error setting metatable for " << CelxLua::ClassNames[id] << '\n';
}

// Initialize the metatable for a class; sets the appropriate registry
// entries and __index, leaving the metatable on the stack when done.
void Celx_CreateClassMetatable(lua_State* l, int id)
{
    lua_newtable(l);
    PushClass(l, id);
    lua_pushvalue(l, -2);
    lua_rawset(l, LUA_REGISTRYINDEX); // registry.name = metatable
    lua_pushvalue(l, -1);
    PushClass(l, id);
    lua_rawset(l, LUA_REGISTRYINDEX); // registry.metatable = name

    lua_pushliteral(l, "__index");
    lua_pushvalue(l, -2);
    lua_rawset(l, -3);
}

// Register a class 'method' in the metatable (assumed to be on top of the stack)
void Celx_RegisterMethod(lua_State* l, const char* name, lua_CFunction fn)
{
    lua_pushstring(l, name);
    lua_pushvalue(l, -2);
    lua_pushcclosure(l, fn, 1);
    lua_settable(l, -3);
}


// Verify that an object at location index on the stack is of the
// specified class
bool Celx_istype(lua_State* l, int index, int id)
{
    // get registry[metatable]
    if (!lua_getmetatable(l, index))
        return false;
    lua_rawget(l, LUA_REGISTRYINDEX);

    if (lua_type(l, -1) != LUA_TSTRING)
    {
        cout << "Celx_istype failed!  Unregistered class.\n";
        lua_pop(l, 1);
        return false;
    }

    const char* classname = lua_tostring(l, -1);
    lua_pop(l, 1);
    return classname != nullptr && strcmp(classname, CelxLua::ClassNames[id]) == 0;
}

// Verify that an object at location index on the stack is of the
// specified class and return pointer to userdata
void* Celx_CheckUserData(lua_State* l, int index, int id)
{
    if (Celx_istype(l, index, id))
        return lua_touserdata(l, index);

    return nullptr;
}

// Return the CelestiaCore object stored in the globals table
CelestiaCore* getAppCore(lua_State* l, FatalErrors fatalErrors)
{
    lua_pushstring(l, "celestia-appcore");
    lua_gettable(l, LUA_REGISTRYINDEX);

    if (!lua_islightuserdata(l, -1))
    {
        if (fatalErrors == NoErrors)
            return nullptr;

        lua_pushstring(l, "internal error: invalid appCore");
        lua_error(l);
    }

    CelestiaCore* appCore = static_cast<CelestiaCore*>(lua_touserdata(l, -1));
    lua_pop(l, 1);
    return appCore;
}


LuaState::LuaState() :
    timeout(MaxTimeslice)
{
#if LUA_VER >= 0x050100
    state = luaL_newstate();
#else
    state = lua_open();
#endif
    timer = new Timer();
    screenshotCount = 0;
}

LuaState::~LuaState()
{
    delete timer;
    if (state != nullptr)
        lua_close(state);
#if 0
    if (costate != nullptr)
        lua_close(costate);
#endif
}

lua_State* LuaState::getState() const
{
    return state;
}


double LuaState::getTime() const
{
    return timer->getTime();
}


// Check if the running script has exceeded its allowed timeslice
// and terminate it if it has:
static void checkTimeslice(lua_State* l, lua_Debug* /*ar*/)
{
    lua_pushstring(l, "celestia-luastate");
    lua_gettable(l, LUA_REGISTRYINDEX);
    if (!lua_islightuserdata(l, -1))
    {
        lua_pushstring(l, "Internal Error: Invalid table entry in checkTimeslice");
        lua_error(l);
    }
    LuaState* luastate = static_cast<LuaState*>(lua_touserdata(l, -1));
    if (luastate == nullptr)
    {
        lua_pushstring(l, "Internal Error: Invalid value in checkTimeslice");
        lua_error(l);
        return;
    }

    if (luastate->timesliceExpired())
    {
        const char* errormsg = "Timeout: script hasn't returned control to celestia (forgot to call wait()?)";
        cerr << errormsg << "\n";
        lua_pushstring(l, errormsg);
        lua_error(l);
    }
}


// allow the script to perform cleanup
void LuaState::cleanup()
{
    if (ioMode == Asking)
    {
        // Restore renderflags:
        CelestiaCore* appCore = getAppCore(costate, NoErrors);
        if (appCore != nullptr)
        {
            lua_pushstring(state, "celestia-savedrenderflags");
            lua_gettable(state, LUA_REGISTRYINDEX);
            if (lua_isuserdata(state, -1))
            {
                uint64_t* savedrenderflags = static_cast<uint64_t*>(lua_touserdata(state, -1));
                appCore->getRenderer()->setRenderFlags(*savedrenderflags);
                // now delete entry:
                lua_pushstring(state, "celestia-savedrenderflags");
                lua_pushnil(state);
                lua_settable(state, LUA_REGISTRYINDEX);
            }
            lua_pop(state,1);
        }
    }
    lua_getglobal(costate, CleanupCallback);
    if (lua_isnil(costate, -1))
        return;

    timeout = getTime() + 1.0;
    if (lua_pcall(costate, 0, 0, 0) != 0)
        cerr << "Error while executing cleanup-callback: " << lua_tostring(costate, -1) << "\n";
}


bool LuaState::createThread()
{
    // Initialize the coroutine which wraps the script
    if (!(lua_isfunction(state, -1) && !lua_iscfunction(state, -1)))
    {
        // Should never happen; we manually set up the stack in C++
        assert(0);
        return false;
    }

    costate = lua_newthread(state);
    if (costate == nullptr)
        return false;

    lua_sethook(costate, checkTimeslice, LUA_MASKCOUNT, 1000);
    lua_pushvalue(state, -2);
    lua_xmove(state, costate, 1);  // move function from L to NL/
    alive = true;
    return true;
}


string LuaState::getErrorMessage()
{
    if (lua_gettop(state) > 0 && lua_isstring(state, -1))
        return lua_tostring(state, -1);

    return "";
}


bool LuaState::timesliceExpired()
{
    if (timeout < getTime())
    {
        // timeslice expired, make every instruction (including pcall) fail:
        lua_sethook(costate, checkTimeslice, LUA_MASKCOUNT, 1);
        return true;
    }

    return false;
}


static int resumeLuaThread(lua_State *L, lua_State *co, int narg)
{
    int status;

    //if (!lua_checkstack(co, narg))
    //   luaL_error(L, "too many arguments to resume");
    lua_xmove(L, co, narg);
#if LUA_VER >= 0x050200
    status = lua_resume(co, nullptr, narg);
#else
    status = lua_resume(co, narg);
#endif
#if LUA_VER >= 0x050100
    if (status == 0 || status == LUA_YIELD)
#else
    if (status == 0)
#endif
    {
        int nres = lua_gettop(co);
        //if (!lua_checkstack(L, narg))
        //   luaL_error(L, "too many results to resume");
        lua_xmove(co, L, nres);  // move yielded values
        return nres;
    }

    lua_xmove(co, L, 1);  // move error message
    return -1;            // error flag
}


bool LuaState::isAlive() const
{
    return alive;
}


struct ReadChunkInfo
{
    char* buf;
    int bufSize;
    istream* in;
};

static const char* readStreamChunk(lua_State* /*unused*/, void* udata, size_t* size)
{
    assert(udata != nullptr);
    if (udata == nullptr)
        return nullptr;

    auto* info = reinterpret_cast<ReadChunkInfo*>(udata);
    assert(info->buf != nullptr);
    assert(info->in != nullptr);

    if (!info->in->good())
    {
        *size = 0;
        return nullptr;
    }

    info->in->read(info->buf, info->bufSize);
    streamsize nread = info->in->gcount();

    *size = (size_t) nread;
    if (nread == 0)
        return nullptr;

    return info->buf;
}


// Callback for CelestiaCore::charEntered.
// Returns true if keypress has been consumed
bool LuaState::charEntered(const char* c_p)
{
    if (ioMode == Asking && getTime() > timeout)
    {
        int stackTop = lua_gettop(costate);
        if (c_p[0] == 'y')
        {
#if LUA_VER >= 0x050100
            openLuaLibrary(costate, LUA_LOADLIBNAME, luaopen_package);
            openLuaLibrary(costate, LUA_IOLIBNAME, luaopen_io);
            openLuaLibrary(costate, LUA_OSLIBNAME, luaopen_os);
#else
            lua_iolibopen(costate);
#endif
            ioMode = IOAllowed;
        }
        else
        {
            ioMode = IODenied;
        }
        CelestiaCore* appCore = getAppCore(costate, NoErrors);
        if (appCore == nullptr)
        {
            cerr << "ERROR: appCore not found\n";
            return true;
        }
        appCore->setTextEnterMode(appCore->getTextEnterMode() & ~CelestiaCore::KbPassToScript);
        appCore->showText("", 0, 0, 0, 0);
        // Restore renderflags:
        lua_pushstring(costate, "celestia-savedrenderflags");
        lua_gettable(costate, LUA_REGISTRYINDEX);
        if (lua_isuserdata(costate, -1))
        {
            uint64_t* savedrenderflags = static_cast<uint64_t*>(lua_touserdata(costate, -1));
            appCore->getRenderer()->setRenderFlags(*savedrenderflags);
            // now delete entry:
            lua_pushstring(costate, "celestia-savedrenderflags");
            lua_pushnil(costate);
            lua_settable(costate, LUA_REGISTRYINDEX);
        }
        else
        {
            cerr << "Oops, expected savedrenderflags to be userdata\n";
        }
        lua_settop(costate,stackTop);
        return true;
    }
#if LUA_VER < 0x050100
    int stack_top = lua_gettop(costate);
#endif
    bool result = true;
    lua_getglobal(costate, KbdCallback);
    lua_pushstring(costate, c_p);
    timeout = getTime() + 1.0;
    if (lua_pcall(costate, 1, 1, 0) != 0)
    {
        cerr << "Error while executing keyboard-callback: " << lua_tostring(costate, -1) << "\n";
        result = false;
    }
    else
    {
        if (lua_isboolean(costate, -1))
        {
            result = (lua_toboolean(costate, -1) != 0);
        }
        lua_pop(costate, 1);
    }

#if LUA_VER < 0x050100
    // cleanup stack - is this necessary?
    lua_settop(costate, stack_top);
#endif
    return result;
}


// Returns true if a handler is registered for the key
bool LuaState::handleKeyEvent(const char* key)
{
    CelestiaCore* appCore = getAppCore(costate, NoErrors);
    if (appCore == nullptr)
        return false;

    // get the registered event table
    getField(costate, LUA_REGISTRYINDEX, EventHandlers);
    if (!lua_istable(costate, -1))
    {
        cerr << "Missing event handler table";
        lua_pop(costate, 1);
        return false;
    }

    bool handled = false;
    getField(costate, -1, KeyHandler);
    if (lua_isfunction(costate, -1))
    {
        lua_remove(costate, -2);        // remove the key event table from the stack

        lua_newtable(costate);
        lua_pushstring(costate, "char");
        lua_pushstring(costate, key);   // the default key handler accepts the key name as an argument
        lua_settable(costate, -3);

        timeout = getTime() + 1.0;
        if (lua_pcall(costate, 1, 1, 0) != 0)
        {
            cerr << "Error while executing keyboard callback: " << lua_tostring(costate, -1) << "\n";
        }
        else
        {
           handled = lua_toboolean(costate, -1) == 1 ? true : false;
        }
        lua_pop(costate, 1);             // pop the return value
    }
    else
    {
        lua_pop(costate, 2);
    }

    return handled;
}


// Returns true if a handler is registered for the button event
bool LuaState::handleMouseButtonEvent(float x, float y, int button, bool down)
{
    CelestiaCore* appCore = getAppCore(costate, NoErrors);
    if (appCore == nullptr)
        return false;

    // get the registered event table
    getField(costate, LUA_REGISTRYINDEX, EventHandlers);
    if (!lua_istable(costate, -1))
    {
        cerr << "Missing event handler table";
        lua_pop(costate, 1);
        return false;
    }

    bool handled = false;
    getField(costate, -1, down ? MouseDownHandler : MouseUpHandler);
    if (lua_isfunction(costate, -1))
    {
        lua_remove(costate, -2);        // remove the key event table from the stack

        lua_newtable(costate);
        lua_pushstring(costate, "button");
        lua_pushnumber(costate, button);
        lua_settable(costate, -3);
        lua_pushstring(costate, "x");
        lua_pushnumber(costate, x);
        lua_settable(costate, -3);
        lua_pushstring(costate, "y");
        lua_pushnumber(costate, y);
        lua_settable(costate, -3);

        timeout = getTime() + 1.0;
        if (lua_pcall(costate, 1, 1, 0) != 0)
        {
            cerr << "Error while executing keyboard callback: " << lua_tostring(costate, -1) << "\n";
        }
        else
        {
           handled = lua_toboolean(costate, -1) == 1 ? true : false;
        }
        lua_pop(costate, 1);             // pop the return value
    }
    else
    {
        lua_pop(costate, 2);
    }

    return handled;
}


// Returns true if a handler is registered for the tick event
bool LuaState::handleTickEvent(double dt)
{
    if (!costate)
        return true;

    CelestiaCore* appCore = getAppCore(costate, NoErrors);
    if (appCore == nullptr)
        return false;

    // get the registered event table
    getField(costate, LUA_REGISTRYINDEX, EventHandlers);
    if (!lua_istable(costate, -1))
    {
        cerr << "Missing event handler table";
        lua_pop(costate, 1);
        return false;
    }

    bool handled = false;
    getField(costate, -1, TickHandler);
    if (lua_isfunction(costate, -1))
    {
        lua_remove(costate, -2);        // remove the key event table from the stack

        lua_newtable(costate);
        lua_pushstring(costate, "dt");
        lua_pushnumber(costate, dt);   // the default key handler accepts the key name as an argument
        lua_settable(costate, -3);

        timeout = getTime() + 1.0;
        if (lua_pcall(costate, 1, 1, 0) != 0)
        {
            cerr << "Error while executing tick callback: " << lua_tostring(costate, -1) << "\n";
        }
        else
        {
           handled = lua_toboolean(costate, -1) == 1 ? true : false;
        }
        lua_pop(costate, 1);             // pop the return value
    }
    else
    {
        lua_pop(costate, 2);
    }

    return handled;
}


int LuaState::loadScript(istream& in, const string& streamname)
{
    char buf[4096];
    ReadChunkInfo info;
    info.buf = buf;
    info.bufSize = sizeof(buf);
    info.in = &in;

    if (streamname != "string")
    {
        lua_pushstring(state, "celestia-scriptpath");
        lua_pushstring(state, streamname.c_str());
        lua_settable(state, LUA_REGISTRYINDEX);
    }

#if LUA_VER >= 0x050200
    int status = lua_load(state, readStreamChunk, &info, streamname.c_str(),
                          nullptr);
#else
    int status = lua_load(state, readStreamChunk, &info, streamname.c_str());
#endif
    if (status != 0)
        cout << "Error loading script: " << lua_tostring(state, -1) << '\n';

    return status;
}

int LuaState::loadScript(const string& s)
{
    istringstream in(s);
    return loadScript(in, "string");
}


// Resume a thread; if the thread completes, the status is set to !alive
int LuaState::resume()
{
    assert(costate != nullptr);
    if (costate == nullptr)
        return 0;

    lua_State* co = lua_tothread(state, -1);
    //assert(co == costate); // co can be nullptr after error (top stack is errorstring)
    if (co != costate)
        return 0;

    timeout = getTime() + MaxTimeslice;
    int nArgs = resumeLuaThread(state, co, 0);
    if (nArgs < 0)
    {
        alive = false;

        const char* errorMessage = lua_tostring(state, -1);
        if (errorMessage == nullptr)
            errorMessage = "Unknown script error";

#if LUA_VER < 0x050100
        // This is a nasty hack required in Lua 5.0, where there's no
        // way to distinguish between a thread returning because it completed
        // or yielded. Thus, we continue to resume the script until we get
        // an error.  The 'cannot resume dead coroutine' error appears when
        // there were no other errors, and execution terminates normally.
        // In Lua 5.1, we can simply check the thread status to find out
        // if it's done executing.
        if (strcmp(errorMessage, "cannot resume dead coroutine") != 0)
#endif
        {
            cout << "Error: " << errorMessage << '\n';
            CelestiaCore* appCore = getAppCore(co);
            if (appCore != nullptr)
            {
                appCore->fatalError(errorMessage);
            }
        }

        return 1; // just the error string
    }

    if (ioMode == Asking)
    {
        // timeout now is used to first only display warning, and 1s
        // later allow response to avoid accidental activation
        timeout = getTime() + 1.0;
    }

#if LUA_VER >= 0x050100
        // The thread status is zero if it has terminated normally
        if (lua_status(co) == 0)
            alive = false;
#endif

    return nArgs; // arguments from yield
}


// get current linenumber of script and create
// useful error-message
void Celx_DoError(lua_State* l, const char* errorMsg)
{
    lua_Debug debug;
    if (lua_getstack(l, 1, &debug))
    {
        if (lua_getinfo(l, "l", &debug))
        {
            string buf = fmt::sprintf("In line %i: %s", debug.currentline, errorMsg);
            lua_pushstring(l, buf.c_str());
            lua_error(l);
        }
    }
    lua_pushstring(l, errorMsg);
    lua_error(l);
}


bool LuaState::tick(double dt)
{
    // Due to the way CelestiaCore::tick is called (at least for KDE),
    // this method may be entered a second time when we show the error-alerter
    // Workaround: check if we are alive, return true(!) when we aren't anymore
    // this way the script isn't deleted after the second enter, but only
    // when we return from the first enter. OMG.

    // better Solution: defer showing the error-alterter to CelestiaCore, using
    // getErrorMessage()
    if (!isAlive())
        return false;

    if (ioMode == Asking)
    {
        CelestiaCore* appCore = getAppCore(costate, NoErrors);
        if (appCore == nullptr)
        {
            cerr << "ERROR: appCore not found\n";
            return true;
        }
        lua_pushstring(state, "celestia-savedrenderflags");
        lua_gettable(state, LUA_REGISTRYINDEX);
        if (lua_isnil(state, -1))
        {
            lua_pushstring(state, "celestia-savedrenderflags");
            uint64_t* savedrenderflags = static_cast<uint64_t*>(lua_newuserdata(state, sizeof(int)));
            *savedrenderflags = appCore->getRenderer()->getRenderFlags();
            lua_settable(state, LUA_REGISTRYINDEX);
            appCore->getRenderer()->setRenderFlags(0);
        }
        // now pop result of gettable
        lua_pop(state, 1);

        if (getTime() > timeout)
        {
            appCore->showText(_("WARNING:\n\nThis script requests permission to read/write files\n"
                              "and execute external programs. Allowing this can be\n"
                              "dangerous.\n"
                              "Do you trust the script and want to allow this?\n\n"
                              "y = yes, ESC = cancel script, any other key = no"),
                              0, 0,
                              -15, 5, 5);
            appCore->setTextEnterMode(appCore->getTextEnterMode() | CelestiaCore::KbPassToScript);
        }
        else
        {
            appCore->showText(_("WARNING:\n\nThis script requests permission to read/write files\n"
                              "and execute external programs. Allowing this can be\n"
                              "dangerous.\n"
                              "Do you trust the script and want to allow this?"),
                              0, 0,
                              -15, 5, 5);
            appCore->setTextEnterMode(appCore->getTextEnterMode() & ~CelestiaCore::KbPassToScript);
        }

        return false;
    }

    if (dt == 0 || scriptAwakenTime > getTime())
        return false;

    int nArgs = resume();
    if (!isAlive()) // The script is complete
        return true;

    // The script has returned control to us, but it is not completed.
    lua_State* state = getState();

    // The values on the stack indicate what event will wake up the
    // script.  For now, we just support wait()
    double delay;
    if (nArgs == 1 && lua_isnumber(state, -1))
        delay = lua_tonumber(state, -1);
    else
        delay = 0.0;
    scriptAwakenTime = getTime() + delay;

    // Clean up the stack
    lua_pop(state, nArgs);
    return false;
}


void LuaState::requestIO()
{
    // the script requested IO, set the mode
    // so we display the warning during tick
    // and can request keyboard. We can't do this now
    // because the script is still active and could
    // disable keyboard again.
    if (ioMode == NoIO)
    {
        CelestiaCore* appCore = getAppCore(state, AllErrors);
        string policy = appCore->getConfig()->scriptSystemAccessPolicy;
        if (policy == "allow")
        {
#if LUA_VER >= 0x050100
            openLuaLibrary(costate, LUA_LOADLIBNAME, luaopen_package);
            openLuaLibrary(costate, LUA_IOLIBNAME, luaopen_io);
            openLuaLibrary(costate, LUA_OSLIBNAME, luaopen_os);
#else
            lua_iolibopen(costate);
#endif
            //luaopen_io(costate);
            ioMode = IOAllowed;
        }
        else if (policy == "deny")
        {
            ioMode = IODenied;
        }
        else
        {
            ioMode = Asking;
        }
    }
}

// Check if the number of arguments on the stack matches
// the allowed range [minArgs, maxArgs]. Cause an error if not.
void Celx_CheckArgs(lua_State* l,
                    int minArgs, int maxArgs, const char* errorMessage)
{
    int argc = lua_gettop(l);
    if (argc < minArgs || argc > maxArgs)
    {
        Celx_DoError(l, errorMessage);
    }
}


ObserverFrame::CoordinateSystem parseCoordSys(const string& name)
{
    // 'planetographic' is a deprecated name for bodyfixed, but maintained here
    // for compatibility with older scripts.

    if (compareIgnoringCase(name, "universal") == 0)
        return ObserverFrame::Universal;
    if (compareIgnoringCase(name, "ecliptic") == 0)
        return ObserverFrame::Ecliptical;
    if (compareIgnoringCase(name, "equatorial") == 0)
        return ObserverFrame::Equatorial;
    if (compareIgnoringCase(name, "bodyfixed") == 0)
        return ObserverFrame::BodyFixed;
    if (compareIgnoringCase(name, "planetographic") == 0)
        return ObserverFrame::BodyFixed;
    if (compareIgnoringCase(name, "observer") == 0)
        return ObserverFrame::ObserverLocal;
    if (compareIgnoringCase(name, "lock") == 0)
        return ObserverFrame::PhaseLock;
    if (compareIgnoringCase(name, "chase") == 0)
        return ObserverFrame::Chase;

    return ObserverFrame::Universal;
}


// Get a pointer to the LuaState-object from the registry:
LuaState* getLuaStateObject(lua_State* l)
{
    int stackSize = lua_gettop(l);
    lua_pushstring(l, "celestia-luastate");
    lua_gettable(l, LUA_REGISTRYINDEX);

    if (!lua_islightuserdata(l, -1))
    {
        Celx_DoError(l, "Internal Error: Invalid table entry for LuaState-pointer");
        return 0;
    }

    LuaState* luastate_ptr = static_cast<LuaState*>(lua_touserdata(l, -1));
    if (luastate_ptr == nullptr)
    {
        Celx_DoError(l, "Internal Error: Invalid LuaState-pointer");
        return 0;
    }

    lua_settop(l, stackSize);
    return luastate_ptr;
}


// Map the observer to its View. Return nullptr if no view exists
// for this observer (anymore).
View* getViewByObserver(CelestiaCore* appCore, Observer* obs)
{
    for (const auto view : appCore->views)
        if (view->observer == obs)
            return view;
    return nullptr;
}

// Fill list with all Observers
void getObservers(CelestiaCore* appCore, vector<Observer*>& observerList)
{
    for (const auto view : appCore->views)
        if (view->type == View::ViewWindow)
            observerList.push_back(view->observer);
}


// ==================== Helpers ====================

// safe wrapper for lua_tostring: fatal errors will terminate script by calling
// lua_error with errorMsg.
const char* Celx_SafeGetString(lua_State* l,
                                      int index,
                                      FatalErrors fatalErrors,
                                      const char* errorMsg)
{
    if (l == nullptr)
    {
        cerr << "Error: LuaState invalid in Celx_SafeGetString\n";
        return nullptr;
    }
    int argc = lua_gettop(l);
    if (index < 1 || index > argc)
    {
        if (fatalErrors & WrongArgc)
            Celx_DoError(l, errorMsg);
        return nullptr;
    }
    if (!lua_isstring(l, index))
    {
        if (fatalErrors & WrongType)
            Celx_DoError(l, errorMsg);
        return nullptr;
    }
    return lua_tostring(l, index);
}

// safe wrapper for lua_tonumber, c.f. Celx_SafeGetString
// Non-fatal errors will return  defaultValue.
lua_Number Celx_SafeGetNumber(lua_State* l, int index, FatalErrors fatalErrors,
                              const char* errorMsg,
                              lua_Number defaultValue)
{
    if (l == nullptr)
    {
        cerr << "Error: LuaState invalid in Celx_SafeGetNumber\n";
        return 0.0;
    }
    int argc = lua_gettop(l);
    if (index < 1 || index > argc)
    {
        if (fatalErrors & WrongArgc)
        {
            Celx_DoError(l, errorMsg);
            return 0;
        }
        return defaultValue;
    }
    if (!lua_isnumber(l, index))
    {
        if (fatalErrors & WrongType)
        {
            Celx_DoError(l, errorMsg);
            return 0;
        }
        return defaultValue;
    }
    return lua_tonumber(l, index);
}

// Safe wrapper for lua_tobool, c.f. safeGetString
// Non-fatal errors will return defaultValue
bool Celx_SafeGetBoolean(lua_State* l, int index, FatalErrors fatalErrors,
                              const char* errorMsg,
                              bool defaultValue)
{
    if (l == nullptr)
    {
        cerr << "Error: LuaState invalid in Celx_SafeGetBoolean\n";
        return false;
    }
    int argc = lua_gettop(l);
    if (index < 1 || index > argc)
    {
        if (fatalErrors & WrongArgc)
        {
            Celx_DoError(l, errorMsg);
            return 0;
        }
        return defaultValue;
    }

    if (!lua_isboolean(l, index))
    {
        if (fatalErrors & WrongType)
        {
            Celx_DoError(l, errorMsg);
            return 0;
        }
        return defaultValue;
    }

    return lua_toboolean(l, index) != 0;
}

// Add a field to the table on top of the stack
void setTable(lua_State* l, const char* field, lua_Number value)
{
    lua_pushstring(l, field);
    lua_pushnumber(l, value);
    lua_settable(l, -3);
}


static void loadLuaLibs(lua_State* state);

// ==================== Initialization ====================
bool LuaState::init(CelestiaCore* appCore)
{
    CelxLua::initMaps();

    // Import the base, table, string, and math libraries
#if LUA_VER >= 0x050100
    openLuaLibrary(state, "", luaopen_base);
    openLuaLibrary(state, LUA_MATHLIBNAME, luaopen_math);
    openLuaLibrary(state, LUA_TABLIBNAME, luaopen_table);
    openLuaLibrary(state, LUA_STRLIBNAME, luaopen_string);
#if LUA_VER >= 0x050200
    openLuaLibrary(state, LUA_COLIBNAME, luaopen_coroutine);
#endif
    // Make the package library, except the loadlib function, available
    // for celx regardless of script system access policy.
    allowLuaPackageAccess();
#else
    lua_baselibopen(state);
    lua_mathlibopen(state);
    lua_tablibopen(state);
    lua_strlibopen(state);
#endif

    // Add an easy to use wait function, so that script writers can
    // live in ignorance of coroutines.  There will probably be a significant
    // library of useful functions that can be defined purely in Lua.
    // At that point, we'll want something a bit more robust than just
    // parsing the whole text of the library every time a script is launched
    if (loadScript("wait = function(x) coroutine.yield(x) end") != 0)
        return false;

    // Execute the script fragment to define the wait function
    if (lua_pcall(state, 0, 0, 0) != 0)
    {
        cout << "Error running script initialization fragment.\n";
        return false;
    }

    lua_pushnumber(state, (lua_Number)KM_PER_LY/1e6);
    lua_setglobal(state, "KM_PER_MICROLY");

    loadLuaLibs(state);

    // Create the celestia object
    celestia_new(state, appCore);
    lua_setglobal(state, "celestia");
    // add reference to appCore in the registry
    lua_pushstring(state, "celestia-appcore");
    lua_pushlightuserdata(state, static_cast<void*>(appCore));
    lua_settable(state, LUA_REGISTRYINDEX);
    // add a reference to the LuaState-object in the registry
    lua_pushstring(state, "celestia-luastate");
    lua_pushlightuserdata(state, static_cast<void*>(this));
    lua_settable(state, LUA_REGISTRYINDEX);

    lua_pushstring(state, EventHandlers);
    lua_newtable(state);
    lua_settable(state, LUA_REGISTRYINDEX);

#if 0
    lua_getglobal(state, "dofile"); // function "dofile" on stack
    lua_pushstring(state, "luainit.celx"); // parameter
    if (lua_pcall(state, 1, 0, 0) != 0) // execute it
    {
        // copy string?!
        const char* errorMessage = lua_tostring(state, -1);
        cout << errorMessage << '\n'; cout.flush();
        appCore->fatalError(errorMessage);
        return false;
    }
#endif

    return true;
}

void LuaState::setLuaPath(const string& s)
{
#if LUA_VER >= 0x050100
    lua_getglobal(state, "package");
    lua_pushstring(state, s.c_str());
    lua_setfield(state, -2, "path");
    lua_pop(state, 1);
#else
    lua_pushstring(state, "LUA_PATH");
    lua_pushstring(state, s.c_str());
    lua_settable(state, LUA_GLOBALSINDEX);
#endif
}


#if LUA_VER < 0x050100
// ======================== loadlib ===================================
/*
* This is an implementation of loadlib based on the dlfcn interface.
* The dlfcn interface is available in Linux, SunOS, Solaris, IRIX, FreeBSD,
* NetBSD, AIX 4.2, HPUX 11, and  probably most other Unix flavors, at least
* as an emulation layer on top of native functions.
*/

#ifndef _WIN32
extern "C" {
#include <lualib.h>
/* #include <lauxlib.h.h> */
#include <dlfcn.h>
}

#if 0
static int x_loadlib(lua_State *L)
{
/* temp -- don't have lauxlib
 const char *path=luaL_checkstring(L,1);
 const char *init=luaL_checkstring(L,2);
*/
cout << "loading lua lib\n"; cout.flush();

 const char *path=lua_tostring(L,1);
 const char *init=lua_tostring(L,2);

 void *lib=dlopen(path,RTLD_NOW);
 if (lib!=nullptr)
 {
  lua_CFunction f=(lua_CFunction) dlsym(lib,init);
  if (f!=nullptr)
  {
   lua_pushlightuserdata(L,lib);
   lua_pushcclosure(L,f,1);
   return 1;
  }
 }
 /* else return appropriate error messages */
 lua_pushnil(L);
 lua_pushstring(L,dlerror());
 lua_pushstring(L,(lib!=nullptr) ? "init" : "open");
 if (lib!=nullptr) dlclose(lib);
 return 3;
}
#endif
#endif // _WIN32
#endif // LUA_VER < 0x050100

// ==================== Load Libraries ================================================

static void loadLuaLibs(lua_State* state)
{
#if LUA_VER >= 0x050100
    openLuaLibrary(state, LUA_DBLIBNAME, luaopen_debug);
#else
    luaopen_debug(state);
#endif

    // TODO: Not required with Lua 5.1
#if 0
#ifndef _WIN32
    lua_pushstring(state, "xloadlib");
    lua_pushcfunction(state, x_loadlib);
    lua_settable(state, LUA_GLOBALSINDEX);
#endif
#endif

    CreateObjectMetaTable(state);
    CreateObserverMetaTable(state);
    CreateCelestiaMetaTable(state);
    CreatePositionMetaTable(state);
    CreateVectorMetaTable(state);
    CreateRotationMetaTable(state);
    CreateFrameMetaTable(state);
    CreatePhaseMetaTable(state);
    CreateCelscriptMetaTable(state);
    CreateFontMetaTable(state);
    CreateImageMetaTable(state);
    CreateTextureMetaTable(state);
    ExtendCelestiaMetaTable(state);
    ExtendObjectMetaTable(state);

    LoadLuaGraphicsLibrary(state);
}


void LuaState::allowSystemAccess()
{
#if LUA_VER >= 0x050100
    openLuaLibrary(state, LUA_LOADLIBNAME, luaopen_package);
    openLuaLibrary(state, LUA_IOLIBNAME, luaopen_io);
    openLuaLibrary(state, LUA_OSLIBNAME, luaopen_os);
#else
    luaopen_io(state);
#endif
    ioMode = IOAllowed;
}


// Permit access to the package library, but prohibit use of the loadlib
// function.
void LuaState::allowLuaPackageAccess()
{
#if LUA_VER >= 0x050100
    openLuaLibrary(state, LUA_LOADLIBNAME, luaopen_package);

    // Disallow loadlib
    lua_getglobal(state, "package");
    lua_pushnil(state);
    lua_setfield(state, -2, "loadlib");
    lua_pop(state, 1);
#endif
}


// ==================== Lua Hook Methods ================================================

void LuaState::setLuaHookEventHandlerEnabled(bool enable)
{
    eventHandlerEnabled = enable;
}


bool LuaState::callLuaHook(void* obj, const char* method)
{
    if (!eventHandlerEnabled)
        return false;

    lua_pushlightuserdata(costate, obj);
    lua_gettable(costate, LUA_REGISTRYINDEX);
    if (!lua_istable(costate, -1))
    {
        lua_pop(costate, 1);
        return false;
    }
    bool handled = false;

    lua_pushstring(costate, method);
    lua_gettable(costate, -2);
    if (lua_isfunction(costate, -1))
    {
        lua_pushvalue(costate, -2);          // push the Lua object the stack
        lua_remove(costate, -3);        // remove the Lua object from the stack

        timeout = getTime() + 1.0;
        if (lua_pcall(costate, 1, 1, 0) != 0)
        {
            cerr << "Error while executing Lua Hook: " << lua_tostring(costate, -1) << "\n";
        }
        else
        {
           handled = lua_toboolean(costate, -1) == 1 ? true : false;
        }
        lua_pop(costate, 1);             // pop the return value
    }
    else
    {
        lua_pop(costate, 2);
    }

    return handled;
}


bool LuaState::callLuaHook(void* obj, const char* method, const char* keyName)
{
    if (!eventHandlerEnabled)
        return false;

    lua_pushlightuserdata(costate, obj);
    lua_gettable(costate, LUA_REGISTRYINDEX);
    if (!lua_istable(costate, -1))
    {
        lua_pop(costate, 1);
        return false;
    }
    bool handled = false;

    lua_pushstring(costate, method);
    lua_gettable(costate, -2);
    if (lua_isfunction(costate, -1))
    {
        lua_pushvalue(costate, -2);          // push the Lua object onto the stack
        lua_remove(costate, -3);             // remove the Lua object from the stack

        lua_pushstring(costate, keyName);    // push the char onto the stack

        timeout = getTime() + 1.0;
        if (lua_pcall(costate, 2, 1, 0) != 0)
        {
            cerr << "Error while executing Lua Hook: " << lua_tostring(costate, -1) << "\n";
        }
        else
        {
           handled = lua_toboolean(costate, -1) == 1 ? true : false;
        }
        lua_pop(costate, 1);             // pop the return value
    }
    else
    {
        lua_pop(costate, 2);
    }

    return handled;
}


bool LuaState::callLuaHook(void* obj, const char* method, float x, float y)
{
    if (!eventHandlerEnabled)
        return false;

    lua_pushlightuserdata(costate, obj);
    lua_gettable(costate, LUA_REGISTRYINDEX);
    if (!lua_istable(costate, -1))
    {
        lua_pop(costate, 1);
        return false;
    }
    bool handled = false;

    lua_pushstring(costate, method);
    lua_gettable(costate, -2);
    if (lua_isfunction(costate, -1))
    {
        lua_pushvalue(costate, -2);          // push the Lua object onto the stack
        lua_remove(costate, -3);        // remove the Lua object from the stack

        lua_pushnumber(costate, x);          // push x onto the stack
        lua_pushnumber(costate, y);          // push y onto the stack

        timeout = getTime() + 1.0;
        if (lua_pcall(costate, 3, 1, 0) != 0)
        {
            cerr << "Error while executing Lua Hook: " << lua_tostring(costate, -1) << "\n";
        }
        else
        {
           handled = lua_toboolean(costate, -1) == 1 ? true : false;
        }
        lua_pop(costate, 1);             // pop the return value
    }
    else
    {
        lua_pop(costate, 2);
    }

    return handled;
}


bool LuaState::callLuaHook(void* obj, const char* method, float x, float y, int b)
{
    if (!eventHandlerEnabled)
        return false;

    lua_pushlightuserdata(costate, obj);
    lua_gettable(costate, LUA_REGISTRYINDEX);
    if (!lua_istable(costate, -1))
    {
        lua_pop(costate, 1);
        return false;
    }
    bool handled = false;

    lua_pushstring(costate, method);
    lua_gettable(costate, -2);
    if (lua_isfunction(costate, -1))
    {
        lua_pushvalue(costate, -2);          // push the Lua object onto the stack
        lua_remove(costate, -3);        // remove the Lua object from the stack

        lua_pushnumber(costate, x);          // push x onto the stack
        lua_pushnumber(costate, y);          // push y onto the stack
        lua_pushnumber(costate, b);          // push b onto the stack

        timeout = getTime() + 1.0;
        if (lua_pcall(costate, 4, 1, 0) != 0)
        {
            cerr << "Error while executing Lua Hook: " << lua_tostring(costate, -1) << "\n";
        }
        else
        {
           handled = lua_toboolean(costate, -1) == 1 ? true : false;
        }
        lua_pop(costate, 1);             // pop the return value
    }
    else
    {
        lua_pop(costate, 2);
    }

    return handled;
}


bool LuaState::callLuaHook(void* obj, const char* method, double dt)
{
    if (!eventHandlerEnabled)
        return false;

    lua_pushlightuserdata(costate, obj);
    lua_gettable(costate, LUA_REGISTRYINDEX);
    if (!lua_istable(costate, -1))
    {
        lua_pop(costate, 1);
        return false;
    }
    bool handled = false;

    lua_pushstring(costate, method);
    lua_gettable(costate, -2);
    if (lua_isfunction(costate, -1))
    {
        lua_pushvalue(costate, -2);          // push the Lua object onto the stack
        lua_remove(costate, -3);             // remove the Lua object from the stack
        lua_pushnumber(costate, dt);

        timeout = getTime() + 1.0;
        if (lua_pcall(costate, 2, 1, 0) != 0)
        {
            cerr << "Error while executing Lua Hook: " << lua_tostring(costate, -1) << "\n";
        }
        else
        {
           handled = lua_toboolean(costate, -1) == 1 ? true : false;
        }
        lua_pop(costate, 1);             // pop the return value
    }
    else
    {
        lua_pop(costate, 2);
    }

    return handled;
}


/**** Implementation of Celx LuaState wrapper ****/

CelxLua::CelxLua(lua_State* l) :
m_lua(l)
{
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


#ifdef __CELVEC__
void CelxLua::newVector(const Vec3d& v)
{
    vector_new(m_lua, v);
}


void CelxLua::newVector(const Vector3d& v)
{
    vector_new(m_lua, fromEigen(v));
}
#else
void CelxLua::newVector(const Vector3d& v)
{
    vector_new(m_lua, v);
}
#endif

void CelxLua::newPosition(const UniversalCoord& uc)
{
    position_new(m_lua, uc);
}


#ifdef __CELVEC__
void CelxLua::newRotation(const Quatd& q)
#else
void CelxLua::newRotation(const Quaterniond& q)
#endif
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

#ifdef __CELVEC__
Vec3d* CelxLua::toVector(int n)
#else
Vector3d* CelxLua::toVector(int n)
#endif
{
    return to_vector(m_lua, n);
}

#ifdef __CELVEC__
Quatd* CelxLua::toRotation(int n)
#else
Quaterniond* CelxLua::toRotation(int n)
#endif
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
            return nullptr;

        lua_pushstring(m_lua, "internal error: invalid appCore");
        lua_error(m_lua);
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
        return 0;
    }

    LuaState* luastate_ptr = static_cast<LuaState*>(lua_touserdata(m_lua, -1));
    if (luastate_ptr == nullptr)
    {
        Celx_DoError(m_lua, "Internal Error: Invalid LuaState-pointer");
        return 0;
    }

    lua_settop(m_lua, stackSize);
    return luastate_ptr;
}
