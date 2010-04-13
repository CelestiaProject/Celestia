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
#include <cstring>
#include <cstdio>
#include <ctime>
#include <map>
#include <celengine/astro.h>
#include <celengine/asterism.h>
#include <celengine/celestia.h>
#include <celengine/cmdparser.h>
#include <celengine/execenv.h>
#include <celengine/execution.h>
#include <celmath/vecmath.h>
#include <celengine/timeline.h>
#include <celengine/timelinephase.h>
#include "imagecapture.h"
#include "url.h"

#include "celx.h"
#include "celx_internal.h"
#include "celx_vector.h"
#include "celx_rotation.h"
#include "celx_position.h"
#include "celx_frame.h"
#include "celx_phase.h"
#include "celx_object.h"
#include "celx_observer.h"
#include "celx_celestia.h"
#include "celx_gl.h"

#include <celengine/eigenport.h>


// Older gcc versions used <strstream> instead of <sstream>.
// This has been corrected in GCC 3.2, but name clashing must
// be avoided
#ifdef __GNUC__
#undef min
#undef max
#endif
#include <sstream>

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

CelxLua::FlagMap CelxLua::RenderFlagMap;
CelxLua::FlagMap CelxLua::LabelFlagMap;
CelxLua::FlagMap CelxLua::LocationFlagMap;
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
static const char* KbdCallback = "celestia_keyboard_callback";
static const char* CleanupCallback = "celestia_cleanup_callback";

static const char* EventHandlers = "celestia_event_handlers";

static const char* KeyHandler        = "key";
static const char* TickHandler       = "tick";
static const char* MouseDownHandler  = "mousedown";
static const char* MouseUpHandler    = "mouseup";


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


#if LUA_VER >= 0x050100
// Load a Lua library--in Lua 5.1, the luaopen_* functions cannot be called
// directly. They most be invoked through the Lua state.
static void openLuaLibrary(lua_State* l,
                           const char* name,
                           lua_CFunction func)
{
    lua_pushcfunction(l, func);
    lua_pushstring(l, name);
    lua_call(l, 1, 0);
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

    if (index != LUA_GLOBALSINDEX && index != LUA_REGISTRYINDEX)
        lua_gettable(l, index - 1);
    else
        lua_gettable(l, index);
}


// Wrapper for a CEL-script, including the needed Execution Environment
class CelScriptWrapper : public ExecutionEnvironment
{
 public:
    CelScriptWrapper(CelestiaCore& appCore, istream& scriptfile):
        script(NULL),
        core(appCore),
        cmdSequence(NULL),
        tickTime(0.0),
        errorMessage("")
    {
        CommandParser parser(scriptfile);
        cmdSequence = parser.parse();
        if (cmdSequence != NULL)
        {
            script = new Execution(*cmdSequence, *this);
        }
        else
        {
            const vector<string>* errors = parser.getErrors();
            if (errors->size() > 0)
                errorMessage = "Error while parsing CEL-script: " + (*errors)[0];
            else
                errorMessage = "Error while parsing CEL-script.";
        }
    }

    virtual ~CelScriptWrapper()
    {
        if (script != NULL)
            delete script;
        if (cmdSequence != NULL)
            delete cmdSequence;
    }

    string getErrorMessage() const
    {
        return errorMessage;
    }

    // tick the CEL-script. t is in seconds and doesn't have to start with zero
    bool tick(double t)
    {
        // use first tick to set the time
        if (tickTime == 0.0)
        {
            tickTime = t;
            return false;
        }
        double dt = t - tickTime;
        tickTime = t;
        return script->tick(dt);
    }

    Simulation* getSimulation() const
    {
        return core.getSimulation();
    }

    Renderer* getRenderer() const
    {
        return core.getRenderer();
    }

    CelestiaCore* getCelestiaCore() const
    {
        return &core;
    }

    void showText(string s, int horig, int vorig, int hoff, int voff,
                  double duration)
    {
        core.showText(s, horig, vorig, hoff, voff, duration);
    }

 private:
    Execution* script;
    CelestiaCore& core;
    CommandSequence* cmdSequence;
    double tickTime;
    string errorMessage;
};


// Push a class name onto the Lua stack
static void PushClass(lua_State* l, int id)
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
static bool Celx_istype(lua_State* l, int index, int id)
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
    if (classname != NULL && strcmp(classname, CelxLua::ClassNames[id]) == 0)
    {
        lua_pop(l, 1);
        return true;
    }
    lua_pop(l, 1);
    return false;
}

// Verify that an object at location index on the stack is of the
// specified class and return pointer to userdata
void* Celx_CheckUserData(lua_State* l, int index, int id)
{
    if (Celx_istype(l, index, id))
        return lua_touserdata(l, index);
    else
        return NULL;
}


// Return the CelestiaCore object stored in the globals table
static CelestiaCore* getAppCore(lua_State* l, FatalErrors fatalErrors = NoErrors)
{
    lua_pushstring(l, "celestia-appcore");
    lua_gettable(l, LUA_REGISTRYINDEX);

    if (!lua_islightuserdata(l, -1))
    {
        if (fatalErrors == NoErrors)
            return NULL;
        else
        {
            lua_pushstring(l, "internal error: invalid appCore");
            lua_error(l);
        }
    }

    CelestiaCore* appCore = static_cast<CelestiaCore*>(lua_touserdata(l, -1));
    lua_pop(l, 1);
    return appCore;
}


LuaState::LuaState() :
    timeout(MaxTimeslice),
    state(NULL),
    costate(NULL),
    alive(false),
    timer(NULL),
    scriptAwakenTime(0.0),
    ioMode(NoIO),
    eventHandlerEnabled(false)
{
    state = lua_open();
    timer = CreateTimer();
    screenshotCount = 0;
}

LuaState::~LuaState()
{
    delete timer;
    if (state != NULL)
        lua_close(state);
#if 0
    if (costate != NULL)
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
    if (luastate == NULL)
    {
        lua_pushstring(l, "Internal Error: Invalid value in checkTimeslice");
        lua_error(l);
    }

    if (luastate->timesliceExpired())
    {
        const char* errormsg = "Timeout: script hasn't returned control to celestia (forgot to call wait()?)";
        cerr << errormsg << "\n";
        lua_pushstring(l, errormsg);
        lua_error(l);
    }
    return;
}


// allow the script to perform cleanup
void LuaState::cleanup()
{
    if (ioMode == Asking)
    {
        // Restore renderflags:
        CelestiaCore* appCore = getAppCore(costate, NoErrors);
        if (appCore != NULL)
        {
            lua_pushstring(state, "celestia-savedrenderflags");
            lua_gettable(state, LUA_REGISTRYINDEX);
            if (lua_isuserdata(state, -1))
            {
                int* savedrenderflags = static_cast<int*>(lua_touserdata(state, -1));
                appCore->getRenderer()->setRenderFlags(*savedrenderflags);
                // now delete entry:
                lua_pushstring(state, "celestia-savedrenderflags");
                lua_pushnil(state);
                lua_settable(state, LUA_REGISTRYINDEX);
            }
            lua_pop(state,1);
        }
    }
    lua_pushstring(costate, CleanupCallback);
    lua_gettable(costate, LUA_GLOBALSINDEX);
    if (lua_isnil(costate, -1))
    {
        return;
    }
    timeout = getTime() + 1.0;
    if (lua_pcall(costate, 0, 0, 0) != 0)
    {
        cerr << "Error while executing cleanup-callback: " << lua_tostring(costate, -1) << "\n";
    }
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
    else
    {
        costate = lua_newthread(state);
        if (costate == NULL)
            return false;
        lua_sethook(costate, checkTimeslice, LUA_MASKCOUNT, 1000);
        lua_pushvalue(state, -2);
        lua_xmove(state, costate, 1);  /* move function from L to NL */
        alive = true;
        return true;
    }
}


string LuaState::getErrorMessage()
{
    if (lua_gettop(state) > 0)
    {
        if (lua_isstring(state, -1))
            return lua_tostring(state, -1);
    }
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
    else
    {
        return false;
    }
}


static int resumeLuaThread(lua_State *L, lua_State *co, int narg)
{
    int status;

    //if (!lua_checkstack(co, narg))
    //   luaL_error(L, "too many arguments to resume");
    lua_xmove(L, co, narg);

    status = lua_resume(co, narg);
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
    else
    {
        lua_xmove(co, L, 1);  // move error message
        return -1;            // error flag
    }
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

static const char* readStreamChunk(lua_State*, void* udata, size_t* size)
{
    assert(udata != NULL);
    if (udata == NULL)
        return NULL;

    ReadChunkInfo* info = reinterpret_cast<ReadChunkInfo*>(udata);
    assert(info->buf != NULL);
    assert(info->in != NULL);

    if (!info->in->good())
    {
        *size = 0;
        return NULL;
    }

    info->in->read(info->buf, info->bufSize);
    streamsize nread = info->in->gcount();

    *size = (size_t) nread;
    if (nread == 0)
        return NULL;
    else
        return info->buf;
}


// Callback for CelestiaCore::charEntered.
// Returns true if keypress has been consumed
bool LuaState::charEntered(const char* c_p)
{
    if (ioMode == Asking && getTime() > timeout)
    {
        int stackTop = lua_gettop(costate);
        if (strcmp(c_p, "y") == 0)
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
        if (appCore == NULL)
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
            int* savedrenderflags = static_cast<int*>(lua_touserdata(costate, -1));
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
    lua_pushstring(costate, KbdCallback);
    lua_gettable(costate, LUA_GLOBALSINDEX);
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
    if (appCore == NULL)
    {
        return false;
    }

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
    if (appCore == NULL)
    {
        return false;
    }

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
    CelestiaCore* appCore = getAppCore(costate, NoErrors);
    if (appCore == NULL)
    {
        return false;
    }

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

    int status = lua_load(state, readStreamChunk, &info, streamname.c_str());
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
    assert(costate != NULL);
    if (costate == NULL)
        return false;

    lua_State* co = lua_tothread(state, -1);
    //assert(co == costate); // co can be NULL after error (top stack is errorstring)
    if (co != costate)
        return false;

    timeout = getTime() + MaxTimeslice;
    int nArgs = resumeLuaThread(state, co, 0);
    if (nArgs < 0)
    {
        alive = false;

        const char* errorMessage = lua_tostring(state, -1);
        if (errorMessage == NULL)
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
            if (appCore != NULL)
            {
                CelestiaCore::Alerter* alerter = appCore->getAlerter();
                alerter->fatalError(errorMessage);
            }
        }

        return 1; // just the error string
    }
    else
    {
        if (ioMode == Asking)
        {
            // timeout now is used to first only display warning, and 1s
            // later allow response to avoid accidental activation
            timeout = getTime() + 1.0;
        }

#if LUA_VER >= 0x050100
        // The thread status is zero if it has terminated normally
        if (lua_status(co) == 0)
        {
            alive = false;
        }
#endif

        return nArgs; // arguments from yield
    }
}


// get current linenumber of script and create
// useful error-message
void Celx_DoError(lua_State* l, const char* errorMsg)
{
    lua_Debug debug;
    if (lua_getstack(l, 1, &debug))
    {
        char buf[1024];
        if (lua_getinfo(l, "l", &debug))
        {
            sprintf(buf, "In line %i: %s", debug.currentline, errorMsg);
            lua_pushstring(l, buf);
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
        if (appCore == NULL)
        {
            cerr << "ERROR: appCore not found\n";
            return true;
        }
        lua_pushstring(state, "celestia-savedrenderflags");
        lua_gettable(state, LUA_REGISTRYINDEX);
        if (lua_isnil(state, -1))
        {
            lua_pushstring(state, "celestia-savedrenderflags");
            int* savedrenderflags = static_cast<int*>(lua_newuserdata(state, sizeof(int)));
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
    if (!isAlive())
    {
        // The script is complete
        return true;
    }
    else
    {
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


static ObserverFrame::CoordinateSystem parseCoordSys(const string& name)
{
	// 'planetographic' is a deprecated name for bodyfixed, but maintained here
	// for compatibility with older scripts.

    if (compareIgnoringCase(name, "universal") == 0)
        return ObserverFrame::Universal;
    else if (compareIgnoringCase(name, "ecliptic") == 0)
        return ObserverFrame::Ecliptical;
    else if (compareIgnoringCase(name, "equatorial") == 0)
        return ObserverFrame::Equatorial;
    else if (compareIgnoringCase(name, "bodyfixed") == 0)
        return ObserverFrame::BodyFixed;
    else if (compareIgnoringCase(name, "planetographic") == 0)
        return ObserverFrame::BodyFixed;
    else if (compareIgnoringCase(name, "observer") == 0)
        return ObserverFrame::ObserverLocal;
    else if (compareIgnoringCase(name, "lock") == 0)
        return ObserverFrame::PhaseLock;
    else if (compareIgnoringCase(name, "chase") == 0)
        return ObserverFrame::Chase;
    else
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
    }
    LuaState* luastate_ptr = static_cast<LuaState*>(lua_touserdata(l, -1));
    if (luastate_ptr == NULL)
    {
        Celx_DoError(l, "Internal Error: Invalid LuaState-pointer");
    }
    lua_settop(l, stackSize);
    return luastate_ptr;
}


// Map the observer to its View. Return NULL if no view exists
// for this observer (anymore).
View* getViewByObserver(CelestiaCore* appCore, Observer* obs)
{
    for (list<View*>::iterator i = appCore->views.begin(); i != appCore->views.end(); i++)
        if ((*i)->observer == obs)
            return *i;
    return NULL;
}

// Fill list with all Observers
void getObservers(CelestiaCore* appCore, vector<Observer*>& observerList)
{
    for (list<View*>::iterator i = appCore->views.begin(); i != appCore->views.end(); i++)
        if ((*i)->type == View::ViewWindow)
            observerList.push_back((*i)->observer);
}


// ==================== Helpers ====================

// safe wrapper for lua_tostring: fatal errors will terminate script by calling
// lua_error with errorMsg.
static const char* Celx_SafeGetString(lua_State* l,
                                      int index,
                                      FatalErrors fatalErrors = AllErrors,
                                      const char* errorMsg = "String argument expected")
{
    if (l == NULL)
    {
        cerr << "Error: LuaState invalid in Celx_SafeGetString\n";
        cout << "Error: LuaState invalid in Celx_SafeGetString\n";
        return NULL;
    }
    int argc = lua_gettop(l);
    if (index < 1 || index > argc)
    {
        if (fatalErrors & WrongArgc)
        {
            Celx_DoError(l, errorMsg);
        }
        else
            return NULL;
    }
    if (!lua_isstring(l, index))
    {
        if (fatalErrors & WrongType)
        {
            Celx_DoError(l, errorMsg);
        }
        else
            return NULL;
    }
    return lua_tostring(l, index);
}

// safe wrapper for lua_tonumber, c.f. Celx_SafeGetString
// Non-fatal errors will return  defaultValue.
lua_Number Celx_SafeGetNumber(lua_State* l, int index, FatalErrors fatalErrors = AllErrors,
                              const char* errorMsg = "Numeric argument expected",
                              lua_Number defaultValue = 0.0)
{
    if (l == NULL)
    {
        cerr << "Error: LuaState invalid in Celx_SafeGetNumber\n";
        cout << "Error: LuaState invalid in Celx_SafeGetNumber\n";
        return 0.0;
    }
    int argc = lua_gettop(l);
    if (index < 1 || index > argc)
    {
        if (fatalErrors & WrongArgc)
        {
            Celx_DoError(l, errorMsg);
        }
        else
            return defaultValue;
    }
    if (!lua_isnumber(l, index))
    {
        if (fatalErrors & WrongType)
        {
            Celx_DoError(l, errorMsg);
        }
        else
            return defaultValue;
    }
    return lua_tonumber(l, index);
}

// Safe wrapper for lua_tobool, c.f. safeGetString
// Non-fatal errors will return defaultValue
bool Celx_SafeGetBoolean(lua_State* l, int index, FatalErrors fatalErrors = AllErrors,
                              const char* errorMsg = "Boolean argument expected",
                              bool defaultValue = false)
{
    if (l == NULL)
    {
        cerr << "Error: LuaState invalid in Celx_SafeGetBoolean\n";
        cout << "Error: LuaState invalid in Celx_SafeGetBoolean\n";
        return 0.0;
    }
    int argc = lua_gettop(l);
    if (index < 1 || index > argc)
    {
        if (fatalErrors & WrongArgc)
        {
            Celx_DoError(l, errorMsg);
        }
        else
        {
            return defaultValue;
        }
    }
    
    if (!lua_isboolean(l, index))
    {
        if (fatalErrors & WrongType)
        {
            Celx_DoError(l, errorMsg);
        }
        else
        {
            return defaultValue;
        }
    }
    
    return lua_toboolean(l, index) != 0;
}

// Add a field to the table on top of the stack
static void setTable(lua_State* l, const char* field, lua_Number value)
{
    lua_pushstring(l, field);
    lua_pushnumber(l, value);
    lua_settable(l, -3);
}




// ==================== Celscript-object ====================

// create a CelScriptWrapper from a string:
static int celscript_from_string(lua_State* l, string& script_text)
{
    istringstream scriptfile(script_text);

    CelestiaCore* appCore = getAppCore(l, AllErrors);
    CelScriptWrapper* celscript = new CelScriptWrapper(*appCore, scriptfile);
    if (celscript->getErrorMessage() != "")
    {
        string error = celscript->getErrorMessage();
        delete celscript;
        Celx_DoError(l, error.c_str());
    }
    else
    {
        CelScriptWrapper** ud = reinterpret_cast<CelScriptWrapper**>(lua_newuserdata(l, sizeof(CelScriptWrapper*)));
        *ud = celscript;
        Celx_SetClass(l, Celx_CelScript);
    }

    return 1;
}

static CelScriptWrapper* this_celscript(lua_State* l)
{
    CelScriptWrapper** script = static_cast<CelScriptWrapper**>(Celx_CheckUserData(l, 1, Celx_CelScript));
    if (script == NULL)
    {
        Celx_DoError(l, "Bad CEL-script object!");
    }
    return *script;
}

static int celscript_tostring(lua_State* l)
{
    lua_pushstring(l, "[Celscript]");

    return 1;
}

static int celscript_tick(lua_State* l)
{
    CelScriptWrapper* script = this_celscript(l);
    LuaState* stateObject = getLuaStateObject(l);
    double t = stateObject->getTime();
    lua_pushboolean(l, !(script->tick(t)) );
    return 1;
}

static int celscript_gc(lua_State* l)
{
    CelScriptWrapper* script = this_celscript(l);
    delete script;
    return 0;
}


static void CreateCelscriptMetaTable(lua_State* l)
{
    Celx_CreateClassMetatable(l, Celx_CelScript);

    Celx_RegisterMethod(l, "__tostring", celscript_tostring);
    Celx_RegisterMethod(l, "tick", celscript_tick);
    Celx_RegisterMethod(l, "__gc", celscript_gc);

    lua_pop(l, 1); // remove metatable from stack
}


// ==================== Celestia-object ====================
static int celestia_new(lua_State* l, CelestiaCore* appCore)
{
    CelestiaCore** ud = reinterpret_cast<CelestiaCore**>(lua_newuserdata(l, sizeof(CelestiaCore*)));
    *ud = appCore;

    Celx_SetClass(l, Celx_Celestia);

    return 1;
}

static CelestiaCore* to_celestia(lua_State* l, int index)
{
    CelestiaCore** appCore = static_cast<CelestiaCore**>(Celx_CheckUserData(l, index, Celx_Celestia));
    if (appCore == NULL)
        return NULL;
    else
        return *appCore;
}

static CelestiaCore* this_celestia(lua_State* l)
{
    CelestiaCore* appCore = to_celestia(l, 1);
    if (appCore == NULL)
    {
        Celx_DoError(l, "Bad celestia object!");
    }

    return appCore;
}


static int celestia_flash(lua_State* l)
{
    Celx_CheckArgs(l, 2, 3, "One or two arguments expected to function celestia:flash");

    CelestiaCore* appCore = this_celestia(l);
    const char* s = Celx_SafeGetString(l, 2, AllErrors, "First argument to celestia:flash must be a string");
    double duration = Celx_SafeGetNumber(l, 3, WrongType, "Second argument to celestia:flash must be a number", 1.5);
    if (duration < 0.0)
    {
        duration = 1.5;
    }

    appCore->flash(s, duration);

    return 0;
}

static int celestia_print(lua_State* l)
{
    Celx_CheckArgs(l, 2, 7, "One to six arguments expected to function celestia:print");

    CelestiaCore* appCore = this_celestia(l);
    const char* s = Celx_SafeGetString(l, 2, AllErrors, "First argument to celestia:print must be a string");
    double duration = Celx_SafeGetNumber(l, 3, WrongType, "Second argument to celestia:print must be a number", 1.5);
    int horig = (int)Celx_SafeGetNumber(l, 4, WrongType, "Third argument to celestia:print must be a number", -1.0);
    int vorig = (int)Celx_SafeGetNumber(l, 5, WrongType, "Fourth argument to celestia:print must be a number", -1.0);
    int hoff = (int)Celx_SafeGetNumber(l, 6, WrongType, "Fifth argument to celestia:print must be a number", 0.0);
    int voff = (int)Celx_SafeGetNumber(l, 7, WrongType, "Sixth argument to celestia:print must be a number", 5.0);

    if (duration < 0.0)
    {
        duration = 1.5;
    }

    appCore->showText(s, horig, vorig, hoff, voff, duration);

    return 0;
}

static int celestia_gettextwidth(lua_State* l)
{
    Celx_CheckArgs(l, 2, 2, "One argument expected to function celestia:gettextwidth");

    CelestiaCore* appCore = this_celestia(l);
    const char* s = Celx_SafeGetString(l, 2, AllErrors, "First argument to celestia:gettextwidth must be a string");

    lua_pushnumber(l, appCore->getTextWidth(s));

    return 1;
}

static int celestia_getaltazimuthmode(lua_State* l)
{
    Celx_CheckArgs(l, 1, 1, "No arguments expected for celestia:getaltazimuthmode()");

    CelestiaCore* appCore = this_celestia(l);
    lua_pushboolean(l, appCore->getAltAzimuthMode());

    return 1;
}

static int celestia_setaltazimuthmode(lua_State* l)
{
    Celx_CheckArgs(l, 2, 2, "One argument expected to function celestia:setaltazimuthmode");
    bool enable = false;
    if (lua_isboolean(l, -1))
     {
        enable = lua_toboolean(l, -1) != 0;
    }
    else
    {
        Celx_DoError(l, "Argument for celestia:setaltazimuthmode must be a boolean");
    }

    CelestiaCore* appCore = this_celestia(l);
    appCore->setAltAzimuthMode(enable);
    lua_pop(l, 1);

    return 0;
}

static int celestia_show(lua_State* l)
{
    Celx_CheckArgs(l, 1, 1000, "Wrong number of arguments to celestia:show");
    CelestiaCore* appCore = this_celestia(l);

    int argc = lua_gettop(l);
    int flags = 0;
    for (int i = 2; i <= argc; i++)
    {
        string renderFlag = Celx_SafeGetString(l, i, AllErrors, "Arguments to celestia:show() must be strings");
        if (renderFlag == "lightdelay")
            appCore->setLightDelayActive(true);
        else if (CelxLua::RenderFlagMap.count(renderFlag) > 0)
            flags |= CelxLua::RenderFlagMap[renderFlag];
    }

    Renderer* r = appCore->getRenderer();
    r->setRenderFlags(r->getRenderFlags() | flags);
    appCore->notifyWatchers(CelestiaCore::RenderFlagsChanged);

    return 0;
}

static int celestia_hide(lua_State* l)
{
    Celx_CheckArgs(l, 1, 1000, "Wrong number of arguments to celestia:hide");
    CelestiaCore* appCore = this_celestia(l);

    int argc = lua_gettop(l);
    int flags = 0;
    for (int i = 2; i <= argc; i++)
    {
        string renderFlag = Celx_SafeGetString(l, i, AllErrors, "Arguments to celestia:hide() must be strings");
        if (renderFlag == "lightdelay")
            appCore->setLightDelayActive(false);
        else if (CelxLua::RenderFlagMap.count(renderFlag) > 0)
            flags |= CelxLua::RenderFlagMap[renderFlag];
    }

    Renderer* r = appCore->getRenderer();
    r->setRenderFlags(r->getRenderFlags() & ~flags);
    appCore->notifyWatchers(CelestiaCore::RenderFlagsChanged);

    return 0;
}

static int celestia_setrenderflags(lua_State* l)
{
    Celx_CheckArgs(l, 2, 2, "One argument expected for celestia:setrenderflags()");
    CelestiaCore* appCore = this_celestia(l);
    if (!lua_istable(l, 2))
    {
        Celx_DoError(l, "Argument to celestia:setrenderflags() must be a table");
    }

    int renderFlags = appCore->getRenderer()->getRenderFlags();
    lua_pushnil(l);
    while (lua_next(l, -2) != 0)
    {
        string key;
        bool value = false;
        if (lua_isstring(l, -2))
        {
            key = lua_tostring(l, -2);
        }
        else
        {
            Celx_DoError(l, "Keys in table-argument to celestia:setrenderflags() must be strings");
        }
        if (lua_isboolean(l, -1))
        {
            value = lua_toboolean(l, -1) != 0;
        }
        else
        {
            Celx_DoError(l, "Values in table-argument to celestia:setrenderflags() must be boolean");
        }
        if (key == "lightdelay")
        {
            appCore->setLightDelayActive(value);
        }
        else if (CelxLua::RenderFlagMap.count(key) > 0)
        {
            int flag = CelxLua::RenderFlagMap[key];
            if (value)
            {
                renderFlags |= flag;
            }
            else
            {
                renderFlags &= ~flag;
            }
        }
        else
        {
            cerr << "Unknown key: " << key << "\n";
        }
        lua_pop(l,1);
    }
    appCore->getRenderer()->setRenderFlags(renderFlags);
    appCore->notifyWatchers(CelestiaCore::RenderFlagsChanged);

    return 0;
}

static int celestia_getrenderflags(lua_State* l)
{
    Celx_CheckArgs(l, 1, 1, "No arguments expected for celestia:getrenderflags()");
    CelestiaCore* appCore = this_celestia(l);
    lua_newtable(l);
    CelxLua::FlagMap::const_iterator it = CelxLua::RenderFlagMap.begin();
    const int renderFlags = appCore->getRenderer()->getRenderFlags();
    while (it != CelxLua::RenderFlagMap.end())
    {
        string key = it->first;
        lua_pushstring(l, key.c_str());
        lua_pushboolean(l, (it->second & renderFlags) != 0);
        lua_settable(l,-3);
        it++;
    }
    lua_pushstring(l, "lightdelay");
    lua_pushboolean(l, appCore->getLightDelayActive());
    lua_settable(l, -3);
    return 1;
}

int celestia_getscreendimension(lua_State* l)
{
    Celx_CheckArgs(l, 1, 1, "No arguments expected for celestia:getscreendimension()");
    // error checking only:
    this_celestia(l);
    // Get the dimensions of the current viewport
    GLint viewport[4];
    glGetIntegerv(GL_VIEWPORT, viewport);
    lua_pushnumber(l, viewport[2]);
    lua_pushnumber(l, viewport[3]);
    return 2;
}

static int celestia_showlabel(lua_State* l)
{
    Celx_CheckArgs(l, 1, 1000, "Bad method call!");
    CelestiaCore* appCore = this_celestia(l);

    int argc = lua_gettop(l);
    int flags = 0;
    for (int i = 2; i <= argc; i++)
    {
        string labelFlag = Celx_SafeGetString(l, i, AllErrors, "Arguments to celestia:showlabel() must be strings");
        if (CelxLua::LabelFlagMap.count(labelFlag) > 0)
            flags |= CelxLua::LabelFlagMap[labelFlag];
    }

    Renderer* r = appCore->getRenderer();
    r->setLabelMode(r->getLabelMode() | flags);
    appCore->notifyWatchers(CelestiaCore::LabelFlagsChanged);

    return 0;
}

static int celestia_hidelabel(lua_State* l)
{
    Celx_CheckArgs(l, 1, 1000, "Invalid number of arguments in celestia:hidelabel");
    CelestiaCore* appCore = this_celestia(l);

    int argc = lua_gettop(l);
    int flags = 0;
    for (int i = 2; i <= argc; i++)
    {
        string labelFlag = Celx_SafeGetString(l, i, AllErrors, "Arguments to celestia:hidelabel() must be strings");
        if (CelxLua::LabelFlagMap.count(labelFlag) > 0)
            flags |= CelxLua::LabelFlagMap[labelFlag];
    }

    Renderer* r = appCore->getRenderer();
    r->setLabelMode(r->getLabelMode() & ~flags);
    appCore->notifyWatchers(CelestiaCore::LabelFlagsChanged);

    return 0;
}

static int celestia_setlabelflags(lua_State* l)
{
    Celx_CheckArgs(l, 2, 2, "One argument expected for celestia:setlabelflags()");
    CelestiaCore* appCore = this_celestia(l);
    if (!lua_istable(l, 2))
    {
        Celx_DoError(l, "Argument to celestia:setlabelflags() must be a table");
    }

    int labelFlags = appCore->getRenderer()->getLabelMode();
    lua_pushnil(l);
    while (lua_next(l, -2) != 0)
    {
        string key;
        bool value = false;
        if (lua_isstring(l, -2))
        {
            key = lua_tostring(l, -2);
        }
        else
        {
            Celx_DoError(l, "Keys in table-argument to celestia:setlabelflags() must be strings");
        }
        if (lua_isboolean(l, -1))
        {
            value = lua_toboolean(l, -1) != 0;
        }
        else
        {
            Celx_DoError(l, "Values in table-argument to celestia:setlabelflags() must be boolean");
        }
        if (CelxLua::LabelFlagMap.count(key) == 0)
        {
            cerr << "Unknown key: " << key << "\n";
        }
        else
        {
            int flag = CelxLua::LabelFlagMap[key];
            if (value)
            {
                labelFlags |= flag;
            }
            else
            {
                labelFlags &= ~flag;
            }
        }
        lua_pop(l,1);
    }
    appCore->getRenderer()->setLabelMode(labelFlags);
    appCore->notifyWatchers(CelestiaCore::LabelFlagsChanged);

    return 0;
}

static int celestia_getlabelflags(lua_State* l)
{
    Celx_CheckArgs(l, 1, 1, "No arguments expected for celestia:getlabelflags()");
    CelestiaCore* appCore = this_celestia(l);
    lua_newtable(l);
    CelxLua::FlagMap::const_iterator it = CelxLua::LabelFlagMap.begin();
    const int labelFlags = appCore->getRenderer()->getLabelMode();
    while (it != CelxLua::LabelFlagMap.end())
    {
        string key = it->first;
        lua_pushstring(l, key.c_str());
        lua_pushboolean(l, (it->second & labelFlags) != 0);
        lua_settable(l,-3);
        it++;
    }
    return 1;
}

static int celestia_setorbitflags(lua_State* l)
{
    Celx_CheckArgs(l, 2, 2, "One argument expected for celestia:setorbitflags()");
    CelestiaCore* appCore = this_celestia(l);
    if (!lua_istable(l, 2))
    {
        Celx_DoError(l, "Argument to celestia:setorbitflags() must be a table");
    }

    int orbitFlags = appCore->getRenderer()->getOrbitMask();
    lua_pushnil(l);
    while (lua_next(l, -2) != 0)
    {
        string key;
        bool value = false;
        if (lua_isstring(l, -2))
        {
            key = lua_tostring(l, -2);
        }
        else
        {
            Celx_DoError(l, "Keys in table-argument to celestia:setorbitflags() must be strings");
        }
        if (lua_isboolean(l, -1))
        {
            value = lua_toboolean(l, -1) != 0;
        }
        else
        {
            Celx_DoError(l, "Values in table-argument to celestia:setorbitflags() must be boolean");
        }
        if (CelxLua::BodyTypeMap.count(key) == 0)
        {
            cerr << "Unknown key: " << key << "\n";
        }
        else
        {
            int flag = CelxLua::BodyTypeMap[key];
            if (value)
            {
                orbitFlags |= flag;
            }
            else
            {
                orbitFlags &= ~flag;
            }
        }
        lua_pop(l,1);
    }
    appCore->getRenderer()->setOrbitMask(orbitFlags);
    return 0;
}

static int celestia_getorbitflags(lua_State* l)
{
    Celx_CheckArgs(l, 1, 1, "No arguments expected for celestia:getorbitflags()");
    CelestiaCore* appCore = this_celestia(l);
    lua_newtable(l);
    CelxLua::FlagMap::const_iterator it = CelxLua::BodyTypeMap.begin();
    const int orbitFlags = appCore->getRenderer()->getOrbitMask();
    while (it != CelxLua::BodyTypeMap.end())
    {
        string key = it->first;
        lua_pushstring(l, key.c_str());
        lua_pushboolean(l, (it->second & orbitFlags) != 0);
        lua_settable(l,-3);
        it++;
    }
    return 1;
}

static int celestia_showconstellations(lua_State* l)
{
    Celx_CheckArgs(l, 1, 2, "Expected no or one argument to celestia:showconstellations()");

    CelestiaCore* appCore = getAppCore(l, AllErrors);
    Universe* u = appCore->getSimulation()->getUniverse();
    AsterismList* asterisms = u->getAsterisms();

    if (lua_type(l, 2) == LUA_TNONE) // No argument passed
    {
        for (AsterismList::const_iterator iter = asterisms->begin();
             iter != asterisms->end(); iter++)
        {
            Asterism* ast = *iter;
            ast->setActive(true);
        }
    }
    else if (!lua_istable(l, 2))
    {
        Celx_DoError(l, "Argument to celestia:showconstellations() must be a table");
    }
    else
    {
        lua_pushnil(l);
        while (lua_next(l, -2) != 0)
        {
            const char* constellation = "";
            if (lua_isstring(l, -1))
            {
                constellation = lua_tostring(l, -1);
            }
            else
            {
                Celx_DoError(l, "Values in table-argument to celestia:showconstellations() must be strings");
            }
            for (AsterismList::const_iterator iter = asterisms->begin();
                 iter != asterisms->end(); iter++)
            {
                Asterism* ast = *iter;
  		          if (compareIgnoringCase(constellation, ast->getName(false)) == 0)
                    ast->setActive(true);
            }
            lua_pop(l,1);
        }
    }

    return 0;
}

static int celestia_hideconstellations(lua_State* l)
{
    Celx_CheckArgs(l, 1, 2, "Expected no or one argument to celestia:hideconstellations()");

    CelestiaCore* appCore = getAppCore(l, AllErrors);
    Universe* u = appCore->getSimulation()->getUniverse();
    AsterismList* asterisms = u->getAsterisms();

    if (lua_type(l, 2) == LUA_TNONE) // No argument passed
    {
        for (AsterismList::const_iterator iter = asterisms->begin();
             iter != asterisms->end(); iter++)
        {
            Asterism* ast = *iter;
            ast->setActive(false);
        }
    }
    else if (!lua_istable(l, 2))
    {
        Celx_DoError(l, "Argument to celestia:hideconstellations() must be a table");
    }
    else
    {
        lua_pushnil(l);
        while (lua_next(l, -2) != 0)
        {
            const char* constellation = "";
            if (lua_isstring(l, -1))
            {
                constellation = lua_tostring(l, -1);
            }
            else
            {
                Celx_DoError(l, "Values in table-argument to celestia:hideconstellations() must be strings");
            }
            for (AsterismList::const_iterator iter = asterisms->begin();
                 iter != asterisms->end(); iter++)
            {
                Asterism* ast = *iter;
  		          if (compareIgnoringCase(constellation, ast->getName(false)) == 0)
                    ast->setActive(false);
            }
            lua_pop(l,1);
        }
    }

    return 0;
}

static int celestia_setconstellationcolor(lua_State* l)
{
    Celx_CheckArgs(l, 4, 5, "Expected three or four arguments to celestia:setconstellationcolor()");

    CelestiaCore* appCore = getAppCore(l, AllErrors);
    Universe* u = appCore->getSimulation()->getUniverse();
    AsterismList* asterisms = u->getAsterisms();

    float r = (float) Celx_SafeGetNumber(l, 2, WrongType, "First argument to celestia:setconstellationcolor() must be a number", 0.0);
    float g = (float) Celx_SafeGetNumber(l, 3, WrongType, "Second argument to celestia:setconstellationcolor() must be a number", 0.0);
    float b = (float) Celx_SafeGetNumber(l, 4, WrongType, "Third argument to celestia:setconstellationcolor() must be a number", 0.0);
    Color constellationColor(r, g, b);

    if (lua_type(l, 5) == LUA_TNONE) // Fourth argument omited
    {
        for (AsterismList::const_iterator iter = asterisms->begin();
             iter != asterisms->end(); iter++)
        {
            Asterism* ast = *iter;
            ast->setOverrideColor(constellationColor);
        }
    }
    else if (!lua_istable(l, 5))
    {
        Celx_DoError(l, "Fourth argument to celestia:setconstellationcolor() must be a table");
    }
    else
    {
        lua_pushnil(l);
        while (lua_next(l, -2) != 0)
        {
            const char* constellation = NULL;
            if (lua_isstring(l, -1))
            {
                constellation = lua_tostring(l, -1);
            }
            else
            {
                Celx_DoError(l, "Values in table-argument to celestia:setconstellationcolor() must be strings");
            }
            for (AsterismList::const_iterator iter = asterisms->begin();
                 iter != asterisms->end(); iter++)
            {
                Asterism* ast = *iter;
  		          if (compareIgnoringCase(constellation, ast->getName(false)) == 0)
                    ast->setOverrideColor(constellationColor);
            }
            lua_pop(l,1);
        }
    }

    return 0;
}

static int celestia_setoverlayelements(lua_State* l)
{
    Celx_CheckArgs(l, 2, 2, "One argument expected for celestia:setoverlayelements()");
    CelestiaCore* appCore = this_celestia(l);
    if (!lua_istable(l, 2))
    {
        Celx_DoError(l, "Argument to celestia:setoverlayelements() must be a table");
    }

    int overlayElements = appCore->getOverlayElements();
    lua_pushnil(l);
    while (lua_next(l, -2) != 0)
    {
        string key;
        bool value = false;
        if (lua_isstring(l, -2))
        {
            key = lua_tostring(l, -2);
        }
        else
        {
            Celx_DoError(l, "Keys in table-argument to celestia:setoverlayelements() must be strings");
        }
        if (lua_isboolean(l, -1))
        {
            value = lua_toboolean(l, -1) != 0;
        }
        else
        {
            Celx_DoError(l, "Values in table-argument to celestia:setoverlayelements() must be boolean");
        }
        if (CelxLua::OverlayElementMap.count(key) == 0)
        {
            cerr << "Unknown key: " << key << "\n";
        }
        else
        {
            int element = CelxLua::OverlayElementMap[key];
            if (value)
            {
                overlayElements |= element;
            }
            else
            {
                overlayElements &= ~element;
            }
        }
        lua_pop(l,1);
    }
    appCore->setOverlayElements(overlayElements);
    return 0;
}

static int celestia_getoverlayelements(lua_State* l)
{
    Celx_CheckArgs(l, 1, 1, "No arguments expected for celestia:getoverlayelements()");
    CelestiaCore* appCore = this_celestia(l);
    lua_newtable(l);
    CelxLua::FlagMap::const_iterator it = CelxLua::OverlayElementMap.begin();
    const int overlayElements = appCore->getOverlayElements();
    while (it != CelxLua::OverlayElementMap.end())
    {
        string key = it->first;
        lua_pushstring(l, key.c_str());
        lua_pushboolean(l, (it->second & overlayElements) != 0);
        lua_settable(l,-3);
        it++;
    }
    return 1;
}


static int celestia_settextcolor(lua_State* l)
{
    Celx_CheckArgs(l, 4, 4, "Three arguments expected for celestia:settextcolor()");
    CelestiaCore* appCore = this_celestia(l);

    Color color;
    double red     = Celx_SafeGetNumber(l, 2, WrongType, "settextcolor: color values must be numbers", 1.0);
    double green   = Celx_SafeGetNumber(l, 3, WrongType, "settextcolor: color values must be numbers", 1.0);
    double blue    = Celx_SafeGetNumber(l, 4, WrongType, "settextcolor: color values must be numbers", 1.0);

    // opacity currently not settable
    double opacity = 1.0;

    color = Color((float) red, (float) green, (float) blue, (float) opacity);
    appCore->setTextColor(color);

    return 0;
}

static int celestia_gettextcolor(lua_State* l)
{
    Celx_CheckArgs(l, 1, 1, "No arguments expected for celestia:getgalaxylightgain()");
    CelestiaCore* appCore = this_celestia(l);

    Color color = appCore->getTextColor();
    lua_pushnumber(l, color.red());
    lua_pushnumber(l, color.green());
    lua_pushnumber(l, color.blue());

    return 3;
}


static int celestia_setlabelcolor(lua_State* l)
{
    Celx_CheckArgs(l, 5, 5, "Four arguments expected for celestia:setlabelcolor()");
    if (!lua_isstring(l, 2))
    {
        Celx_DoError(l, "First argument to celestia:setlabelstyle() must be a string");
    }

    Color* color = NULL;
    string key;
    key = lua_tostring(l, 2);
    if (CelxLua::LabelColorMap.count(key) == 0)
    {
        cerr << "Unknown label style: " << key << "\n";
    }
    else
    {
        color = CelxLua::LabelColorMap[key];
    }

    double red     = Celx_SafeGetNumber(l, 3, AllErrors, "setlabelcolor: color values must be numbers");
    double green   = Celx_SafeGetNumber(l, 4, AllErrors, "setlabelcolor: color values must be numbers");
    double blue    = Celx_SafeGetNumber(l, 5, AllErrors, "setlabelcolor: color values must be numbers");

    // opacity currently not settable
    double opacity = 1.0;

    if (color != NULL)
    {
        *color = Color((float) red, (float) green, (float) blue, (float) opacity);
    }

    return 1;
}


static int celestia_getlabelcolor(lua_State* l)
{
    Celx_CheckArgs(l, 2, 2, "One argument expected for celestia:getlabelcolor()");
    string key = Celx_SafeGetString(l, 2, AllErrors, "Argument to celestia:getlabelcolor() must be a string");

    Color* labelColor = NULL;
    if (CelxLua::LabelColorMap.count(key) == 0)
    {
        cerr << "Unknown label style: " << key << "\n";
        return 0;
    }
    else
    {
        labelColor = CelxLua::LabelColorMap[key];
        lua_pushnumber(l, labelColor->red());
        lua_pushnumber(l, labelColor->green());
        lua_pushnumber(l, labelColor->blue());

        return 3;
    }
}


static int celestia_setlinecolor(lua_State* l)
{
    Celx_CheckArgs(l, 5, 5, "Four arguments expected for celestia:setlinecolor()");
    if (!lua_isstring(l, 2))
    {
        Celx_DoError(l, "First argument to celestia:setlinecolor() must be a string");
    }

    Color* color = NULL;
    string key;
    key = lua_tostring(l, 2);
    if (CelxLua::LineColorMap.count(key) == 0)
    {
        cerr << "Unknown line style: " << key << "\n";
    }
    else
    {
        color = CelxLua::LineColorMap[key];
    }

    double red     = Celx_SafeGetNumber(l, 3, AllErrors, "setlinecolor: color values must be numbers");
    double green   = Celx_SafeGetNumber(l, 4, AllErrors, "setlinecolor: color values must be numbers");
    double blue    = Celx_SafeGetNumber(l, 5, AllErrors, "setlinecolor: color values must be numbers");

    // opacity currently not settable
    double opacity = 1.0;

    if (color != NULL)
    {
        *color = Color((float) red, (float) green, (float) blue, (float) opacity);
    }

    return 1;
}


static int celestia_getlinecolor(lua_State* l)
{
    Celx_CheckArgs(l, 2, 2, "One argument expected for celestia:getlinecolor()");
    string key = Celx_SafeGetString(l, 2, AllErrors, "Argument to celestia:getlinecolor() must be a string");

    Color* lineColor = NULL;
    if (CelxLua::LineColorMap.count(key) == 0)
    {
        cerr << "Unknown line style: " << key << "\n";
        return 0;
    }
    else
    {
        lineColor = CelxLua::LineColorMap[key];
        lua_pushnumber(l, lineColor->red());
        lua_pushnumber(l, lineColor->green());
        lua_pushnumber(l, lineColor->blue());

        return 3;
    }
}


static int celestia_setfaintestvisible(lua_State* l)
{
    Celx_CheckArgs(l, 2, 2, "One argument expected for celestia:setfaintestvisible()");
    CelestiaCore* appCore = this_celestia(l);
    float faintest = (float)Celx_SafeGetNumber(l, 2, AllErrors, "Argument to celestia:setfaintestvisible() must be a number");
    if ((appCore->getRenderer()->getRenderFlags() & Renderer::ShowAutoMag) == 0)
    {
        faintest = min(15.0f, max(1.0f, faintest));
        appCore->setFaintest(faintest);
        appCore->notifyWatchers(CelestiaCore::FaintestChanged);
    }
    else
    {
        faintest = min(12.0f, max(6.0f, faintest));
        appCore->getRenderer()->setFaintestAM45deg(faintest);
        appCore->setFaintestAutoMag();
    }
    return 0;
}

static int celestia_getfaintestvisible(lua_State* l)
{
    Celx_CheckArgs(l, 1, 1, "No arguments expected for celestia:getfaintestvisible()");
    CelestiaCore* appCore = this_celestia(l);
    if ((appCore->getRenderer()->getRenderFlags() & Renderer::ShowAutoMag) == 0)
    {
        lua_pushnumber(l, appCore->getSimulation()->getFaintestVisible());
    }
    else
    {
        lua_pushnumber(l, appCore->getRenderer()->getFaintestAM45deg());
    }
    return 1;
}

static int celestia_setgalaxylightgain(lua_State* l)
{
    Celx_CheckArgs(l, 2, 2, "One argument expected for celestia:setgalaxylightgain()");
    float lightgain = (float)Celx_SafeGetNumber(l, 2, AllErrors, "Argument to celestia:setgalaxylightgain() must be a number");
    lightgain = min(1.0f, max(0.0f, lightgain));
    Galaxy::setLightGain(lightgain);

    return 0;
}

static int celestia_getgalaxylightgain(lua_State* l)
{
    Celx_CheckArgs(l, 1, 1, "No arguments expected for celestia:getgalaxylightgain()");
    lua_pushnumber(l, Galaxy::getLightGain());

    return 1;
}

static int celestia_setminfeaturesize(lua_State* l)
{
    Celx_CheckArgs(l, 2, 2, "One argument expected for celestia:setminfeaturesize()");
    CelestiaCore* appCore = this_celestia(l);
    float minFeatureSize = (float)Celx_SafeGetNumber(l, 2, AllErrors, "Argument to celestia:setminfeaturesize() must be a number");
    minFeatureSize = max(0.0f, minFeatureSize);
    appCore->getRenderer()->setMinimumFeatureSize(minFeatureSize);
    return 0;
}

static int celestia_getminfeaturesize(lua_State* l)
{
    Celx_CheckArgs(l, 1, 1, "No arguments expected for celestia:getminfeaturesize()");
    CelestiaCore* appCore = this_celestia(l);
    lua_pushnumber(l, appCore->getRenderer()->getMinimumFeatureSize());
    return 1;
}

static int celestia_getobserver(lua_State* l)
{
    Celx_CheckArgs(l, 1, 1, "No arguments expected for celestia:getobserver()");

    CelestiaCore* appCore = this_celestia(l);
    Observer* o = appCore->getSimulation()->getActiveObserver();
    if (o == NULL)
        lua_pushnil(l);
    else
        observer_new(l, o);

    return 1;
}

static int celestia_getobservers(lua_State* l)
{
    Celx_CheckArgs(l, 1, 1, "No arguments expected for celestia:getobservers()");
    CelestiaCore* appCore = this_celestia(l);

    vector<Observer*> observer_list;
    getObservers(appCore, observer_list);
    lua_newtable(l);
    for (unsigned int i = 0; i < observer_list.size(); i++)
    {
        observer_new(l, observer_list[i]);
        lua_rawseti(l, -2, i + 1);
    }

    return 1;
}

static int celestia_getselection(lua_State* l)
{
    Celx_CheckArgs(l, 1, 1, "No arguments expected to celestia:getselection()");
    CelestiaCore* appCore = this_celestia(l);
    Selection sel = appCore->getSimulation()->getSelection();
    object_new(l, sel);

    return 1;
}

static int celestia_find(lua_State* l)
{
    Celx_CheckArgs(l, 2, 2, "One argument expected for function celestia:find()");
    if (!lua_isstring(l, 2))
    {
        Celx_DoError(l, "Argument to find must be a string");
    }

    CelestiaCore* appCore = this_celestia(l);
    Simulation* sim = appCore->getSimulation();
    // Should use universe not simulation for finding objects
    Selection sel = sim->findObjectFromPath(lua_tostring(l, 2));
    object_new(l, sel);

    return 1;
}

static int celestia_select(lua_State* l)
{
    Celx_CheckArgs(l, 2, 2, "One argument expected for celestia:select()");
    CelestiaCore* appCore = this_celestia(l);

    Simulation* sim = appCore->getSimulation();
    Selection* sel = to_object(l, 2);

    // If the argument is an object, set the selection; if it's anything else
    // clear the selection.
    if (sel != NULL)
        sim->setSelection(*sel);
    else
        sim->setSelection(Selection());

    return 0;
}

static int celestia_mark(lua_State* l)
{
    Celx_CheckArgs(l, 2, 2, "One argument expected to function celestia:mark");

    CelestiaCore* appCore = this_celestia(l);
    Simulation* sim = appCore->getSimulation();
    Selection* sel = to_object(l, 2);

    if (sel != NULL)
    {
        MarkerRepresentation markerRep(MarkerRepresentation::Diamond);
        markerRep.setColor(Color(0.0f, 1.0f, 0.0f));
        markerRep.setSize(10.0f);
        
        sim->getUniverse()->markObject(*sel, markerRep, 1);
    }
    else
    {
        Celx_DoError(l, "Argument to celestia:mark must be an object");
    }

    return 0;
}

static int celestia_unmark(lua_State* l)
{
    Celx_CheckArgs(l, 2, 2, "One argument expected to function celestia:unmark");

    CelestiaCore* appCore = this_celestia(l);
    Simulation* sim = appCore->getSimulation();
    Selection* sel = to_object(l, 2);

    if (sel != NULL)
    {
        sim->getUniverse()->unmarkObject(*sel, 1);
    }
    else
    {
        Celx_DoError(l, "Argument to celestia:unmark must be an object");
    }

    return 0;
}

static int celestia_gettime(lua_State* l)
{
    Celx_CheckArgs(l, 1, 1, "No argument expected to function celestia:gettime");

    CelestiaCore* appCore = this_celestia(l);
    Simulation* sim = appCore->getSimulation();
    lua_pushnumber(l, sim->getTime());

    return 1;
}

static int celestia_gettimescale(lua_State* l)
{
    Celx_CheckArgs(l, 1, 1, "No argument expected to function celestia:gettimescale");

    CelestiaCore* appCore = this_celestia(l);
    lua_pushnumber(l, appCore->getSimulation()->getTimeScale());

    return 1;
}

static int celestia_settime(lua_State* l)
{
    Celx_CheckArgs(l, 2, 2, "One argument expected to function celestia:settime");

    CelestiaCore* appCore = this_celestia(l);
    double t = Celx_SafeGetNumber(l, 2, AllErrors, "First arg to celestia:settime must be a number");
    appCore->getSimulation()->setTime(t);

    return 0;
}

static int celestia_ispaused(lua_State* l)
{
    Celx_CheckArgs(l, 1, 1, "No argument expected to function celestia:ispaused");

    CelestiaCore* appCore = this_celestia(l);
    lua_pushboolean(l, appCore->getSimulation()->getPauseState());

    return 1;
}

static int celestia_synchronizetime(lua_State* l)
{
    Celx_CheckArgs(l, 2, 2, "One argument expected to function celestia:synchronizetime");

    CelestiaCore* appCore = this_celestia(l);
    bool sync = Celx_SafeGetBoolean(l, 2, AllErrors, "Argument to celestia:synchronizetime must be a boolean");
    appCore->getSimulation()->setSyncTime(sync);

    return 0;
}

static int celestia_istimesynchronized(lua_State* l)
{
    Celx_CheckArgs(l, 1, 1, "No argument expected to function celestia:istimesynchronized");

    CelestiaCore* appCore = this_celestia(l);
    lua_pushboolean(l, appCore->getSimulation()->getSyncTime());

    return 1;
}


static int celestia_settimescale(lua_State* l)
{
    Celx_CheckArgs(l, 2, 2, "One argument expected to function celestia:settimescale");

    CelestiaCore* appCore = this_celestia(l);
    double t = Celx_SafeGetNumber(l, 2, AllErrors, "Second arg to celestia:settimescale must be a number");
    appCore->getSimulation()->setTimeScale(t);

    return 0;
}

static int celestia_tojulianday(lua_State* l)
{
    Celx_CheckArgs(l, 2, 7, "Wrong number of arguments to function celestia:tojulianday");

    // for error checking only:
    this_celestia(l);

    int year = (int)Celx_SafeGetNumber(l, 2, AllErrors, "First arg to celestia:tojulianday must be a number", 0.0);
    int month = (int)Celx_SafeGetNumber(l, 3, WrongType, "Second arg to celestia:tojulianday must be a number", 1.0);
    int day = (int)Celx_SafeGetNumber(l, 4, WrongType, "Third arg to celestia:tojulianday must be a number", 1.0);
    int hour = (int)Celx_SafeGetNumber(l, 5, WrongType, "Fourth arg to celestia:tojulianday must be a number", 0.0);
    int minute = (int)Celx_SafeGetNumber(l, 6, WrongType, "Fifth arg to celestia:tojulianday must be a number", 0.0);
    double seconds = Celx_SafeGetNumber(l, 7, WrongType, "Sixth arg to celestia:tojulianday must be a number", 0.0);

    astro::Date date(year, month, day);
    date.hour = hour;
    date.minute = minute;
    date.seconds = seconds;

    double jd = (double) date;

    lua_pushnumber(l, jd);

    return 1;
}

static int celestia_fromjulianday(lua_State* l)
{
    Celx_CheckArgs(l, 2, 2, "Wrong number of arguments to function celestia:fromjulianday");

    // for error checking only:
    this_celestia(l);

    double jd = Celx_SafeGetNumber(l, 2, AllErrors, "First arg to celestia:fromjulianday must be a number", 0.0);
    astro::Date date(jd);

    lua_newtable(l);
    setTable(l, "year", (double)date.year);
    setTable(l, "month", (double)date.month);
    setTable(l, "day", (double)date.day);
    setTable(l, "hour", (double)date.hour);
    setTable(l, "minute", (double)date.minute);
    setTable(l, "seconds", date.seconds);

    return 1;
}


// Convert a UTC Julian date to a TDB Julian day
// TODO: also support a single table argument of the form output by
// celestia_tdbtoutc.
static int celestia_utctotdb(lua_State* l)
{
    Celx_CheckArgs(l, 2, 7, "Wrong number of arguments to function celestia:utctotdb");

    // for error checking only:
    this_celestia(l);

    int year = (int) Celx_SafeGetNumber(l, 2, AllErrors, "First arg to celestia:utctotdb must be a number", 0.0);
    int month = (int) Celx_SafeGetNumber(l, 3, WrongType, "Second arg to celestia:utctotdb must be a number", 1.0);
    int day = (int) Celx_SafeGetNumber(l, 4, WrongType, "Third arg to celestia:utctotdb must be a number", 1.0);
    int hour = (int)Celx_SafeGetNumber(l, 5, WrongType, "Fourth arg to celestia:utctotdb must be a number", 0.0);
    int minute = (int)Celx_SafeGetNumber(l, 6, WrongType, "Fifth arg to celestia:utctotdb must be a number", 0.0);
    double seconds = Celx_SafeGetNumber(l, 7, WrongType, "Sixth arg to celestia:utctotdb must be a number", 0.0);

    astro::Date date(year, month, day);
    date.hour = hour;
    date.minute = minute;
    date.seconds = seconds;

    double jd = astro::UTCtoTDB(date);

    lua_pushnumber(l, jd);

    return 1;
}


// Convert a TDB Julian day to a UTC Julian date (table format)
static int celestia_tdbtoutc(lua_State* l)
{
    Celx_CheckArgs(l, 2, 2, "Wrong number of arguments to function celestia:tdbtoutc");

    // for error checking only:
    this_celestia(l);

    double jd = Celx_SafeGetNumber(l, 2, AllErrors, "First arg to celestia:tdbtoutc must be a number", 0.0);
    astro::Date date = astro::TDBtoUTC(jd);

    lua_newtable(l);
    setTable(l, "year", (double)date.year);
    setTable(l, "month", (double)date.month);
    setTable(l, "day", (double)date.day);
    setTable(l, "hour", (double)date.hour);
    setTable(l, "minute", (double)date.minute);
    setTable(l, "seconds", date.seconds);

    return 1;
}


static int celestia_getsystemtime(lua_State* l)
{
    Celx_CheckArgs(l, 1, 1, "No argument expected to function celestia:getsystemtime");
    
    astro::Date d = astro::Date::systemDate();
    lua_pushnumber(l, astro::UTCtoTDB(d));

    return 1;
}


static int celestia_unmarkall(lua_State* l)
{
    Celx_CheckArgs(l, 1, 1, "No arguments expected to function celestia:unmarkall");

    CelestiaCore* appCore = this_celestia(l);
    Simulation* sim = appCore->getSimulation();
    sim->getUniverse()->unmarkAll();

    return 0;
}

static int celestia_getstarcount(lua_State* l)
{
    Celx_CheckArgs(l, 1, 1, "No arguments expected to function celestia:getstarcount");

    CelestiaCore* appCore = this_celestia(l);
    Universe* u = appCore->getSimulation()->getUniverse();
    lua_pushnumber(l, u->getStarCatalog()->size());

    return 1;
}


// Stars iterator function; two upvalues expected
static int celestia_stars_iter(lua_State* l)
{
    CelestiaCore* appCore = to_celestia(l, lua_upvalueindex(1));
    if (appCore == NULL)
    {
        Celx_DoError(l, "Bad celestia object!");
        return 0;
    }

    uint32 i = (uint32) lua_tonumber(l, lua_upvalueindex(2));
    Universe* u = appCore->getSimulation()->getUniverse();

    if (i < u->getStarCatalog()->size())
    {
        // Increment the counter
        lua_pushnumber(l, i + 1);
        lua_replace(l, lua_upvalueindex(2));

        Star* star = u->getStarCatalog()->getStar(i);
        if (star == NULL)
            lua_pushnil(l);
        else
            object_new(l, Selection(star));

        return 1;
    }
    else
    {
        // Return nil when we've enumerated all the stars
        return 0;
    }
}


static int celestia_stars(lua_State* l)
{
    // Push a closure with two upvalues: the celestia object and a
    // counter.
    lua_pushvalue(l, 1);    // Celestia object
    lua_pushnumber(l, 0);   // counter
    lua_pushcclosure(l, celestia_stars_iter, 2);

    return 1;
}


static int celestia_getdsocount(lua_State* l)
{
    Celx_CheckArgs(l, 1, 1, "No arguments expected to function celestia:getdsocount");

    CelestiaCore* appCore = this_celestia(l);
    Universe* u = appCore->getSimulation()->getUniverse();
    lua_pushnumber(l, u->getDSOCatalog()->size());

    return 1;
}


// DSOs iterator function; two upvalues expected
static int celestia_dsos_iter(lua_State* l)
{
    CelestiaCore* appCore = to_celestia(l, lua_upvalueindex(1));
    if (appCore == NULL)
    {
        Celx_DoError(l, "Bad celestia object!");
        return 0;
    }

    uint32 i = (uint32) lua_tonumber(l, lua_upvalueindex(2));
    Universe* u = appCore->getSimulation()->getUniverse();

    if (i < u->getDSOCatalog()->size())
    {
        // Increment the counter
        lua_pushnumber(l, i + 1);
        lua_replace(l, lua_upvalueindex(2));

        DeepSkyObject* dso = u->getDSOCatalog()->getDSO(i);
        if (dso == NULL)
            lua_pushnil(l);
        else
            object_new(l, Selection(dso));

        return 1;
    }
    else
    {
        // Return nil when we've enumerated all the DSOs
        return 0;
    }
}


static int celestia_dsos(lua_State* l)
{
    // Push a closure with two upvalues: the celestia object and a
    // counter.
    lua_pushvalue(l, 1);    // Celestia object
    lua_pushnumber(l, 0);   // counter
    lua_pushcclosure(l, celestia_dsos_iter, 2);

    return 1;
}

static int celestia_setambient(lua_State* l)
{
    Celx_CheckArgs(l, 2, 2, "One argument expected in celestia:setambient");
    CelestiaCore* appCore = this_celestia(l);

    Renderer* renderer = appCore->getRenderer();
    double ambientLightLevel = Celx_SafeGetNumber(l, 2, AllErrors, "Argument to celestia:setambient must be a number");
    if (ambientLightLevel > 1.0)
        ambientLightLevel = 1.0;
    if (ambientLightLevel < 0.0)
        ambientLightLevel = 0.0;

    if (renderer != NULL)
        renderer->setAmbientLightLevel((float)ambientLightLevel);
    appCore->notifyWatchers(CelestiaCore::AmbientLightChanged);

    return 0;
}

static int celestia_getambient(lua_State* l)
{
    Celx_CheckArgs(l, 1, 1, "No argument expected in celestia:setambient");
    CelestiaCore* appCore = this_celestia(l);

    Renderer* renderer = appCore->getRenderer();
    if (renderer == NULL)
    {
        Celx_DoError(l, "Internal Error: renderer is NULL!");
    }
    else
    {
        lua_pushnumber(l, renderer->getAmbientLightLevel());
    }
    return 1;
}

static int celestia_setminorbitsize(lua_State* l)
{
    Celx_CheckArgs(l, 2, 2, "One argument expected in celestia:setminorbitsize");
    CelestiaCore* appCore = this_celestia(l);

    double orbitSize = Celx_SafeGetNumber(l, 2, AllErrors, "Argument to celestia:setminorbitsize() must be a number");
    Renderer* renderer = appCore->getRenderer();
    if (renderer == NULL)
    {
        Celx_DoError(l, "Internal Error: renderer is NULL!");
    }
    else
    {
        orbitSize = max(0.0, orbitSize);
        renderer->setMinimumOrbitSize((float)orbitSize);
    }
    return 0;
}

static int celestia_getminorbitsize(lua_State* l)
{
    Celx_CheckArgs(l, 1, 1, "No argument expected in celestia:getminorbitsize");
    CelestiaCore* appCore = this_celestia(l);

    Renderer* renderer = appCore->getRenderer();
    if (renderer == NULL)
    {
        Celx_DoError(l, "Internal Error: renderer is NULL!");
    }
    else
    {
        lua_pushnumber(l, renderer->getMinimumOrbitSize());
    }
    return 1;
}

static int celestia_setstardistancelimit(lua_State* l)
{
    Celx_CheckArgs(l, 2, 2, "One argument expected in celestia:setstardistancelimit");
    CelestiaCore* appCore = this_celestia(l);

    double distanceLimit = Celx_SafeGetNumber(l, 2, AllErrors, "Argument to celestia:setstardistancelimit() must be a number");
    Renderer* renderer = appCore->getRenderer();
    if (renderer == NULL)
    {
        Celx_DoError(l, "Internal Error: renderer is NULL!");
    }
    else
    {
        renderer->setDistanceLimit((float)distanceLimit);
    }
    return 0;
}

static int celestia_getstardistancelimit(lua_State* l)
{
    Celx_CheckArgs(l, 1, 1, "No argument expected in celestia:getstardistancelimit");
    CelestiaCore* appCore = this_celestia(l);

    Renderer* renderer = appCore->getRenderer();
    if (renderer == NULL)
    {
        Celx_DoError(l, "Internal Error: renderer is NULL!");
    }
    else
    {
        lua_pushnumber(l, renderer->getDistanceLimit());
    }
    return 1;
}

static int celestia_getstarstyle(lua_State* l)
{
    Celx_CheckArgs(l, 1, 1, "No argument expected in celestia:getstarstyle");
    CelestiaCore* appCore = this_celestia(l);

    Renderer* renderer = appCore->getRenderer();
    if (renderer == NULL)
    {
        Celx_DoError(l, "Internal Error: renderer is NULL!");
    }
    else
    {
        Renderer::StarStyle starStyle = renderer->getStarStyle();
        switch (starStyle)
        {
        case Renderer::FuzzyPointStars:
            lua_pushstring(l, "fuzzy"); break;
        case Renderer::PointStars:
            lua_pushstring(l, "point"); break;
        case Renderer::ScaledDiscStars:
            lua_pushstring(l, "disc"); break;
        default:
            lua_pushstring(l, "invalid starstyle");
        };
    }
    return 1;
}

static int celestia_setstarstyle(lua_State* l)
{
    Celx_CheckArgs(l, 2, 2, "One argument expected in celestia:setstarstyle");
    CelestiaCore* appCore = this_celestia(l);

    string starStyle = Celx_SafeGetString(l, 2, AllErrors, "Argument to celestia:setstarstyle must be a string");
    Renderer* renderer = appCore->getRenderer();
    if (renderer == NULL)
    {
        Celx_DoError(l, "Internal Error: renderer is NULL!");
    }
    else
    {
        if (starStyle == "fuzzy")
        {
            renderer->setStarStyle(Renderer::FuzzyPointStars);
        }
        else if (starStyle == "point")
        {
            renderer->setStarStyle(Renderer::PointStars);
        }
        else if (starStyle == "disc")
        {
            renderer->setStarStyle(Renderer::ScaledDiscStars);
        }
        else
        {
            Celx_DoError(l, "Invalid starstyle");
        }
        appCore->notifyWatchers(CelestiaCore::RenderFlagsChanged);

    }
    return 0;
}

static int celestia_gettextureresolution(lua_State* l)
{
    Celx_CheckArgs(l, 1, 1, "No argument expected in celestia:gettextureresolution");
    CelestiaCore* appCore = this_celestia(l);

    Renderer* renderer = appCore->getRenderer();
    if (renderer == NULL)
        Celx_DoError(l, "Internal Error: renderer is NULL!");
    else
        lua_pushnumber(l, renderer->getResolution());

    return 1;
}

static int celestia_settextureresolution(lua_State* l)
{
    Celx_CheckArgs(l, 2, 2, "One argument expected in celestia:settextureresolution");
    CelestiaCore* appCore = this_celestia(l);

    unsigned int textureRes = (unsigned int) Celx_SafeGetNumber(l, 2, AllErrors, "Argument to celestia:settextureresolution must be a number");
    Renderer* renderer = appCore->getRenderer();
    if (renderer == NULL)
        Celx_DoError(l, "Internal Error: renderer is NULL!");
    else
    {
        renderer->setResolution(textureRes);
        appCore->notifyWatchers(CelestiaCore::RenderFlagsChanged);
    }

    return 0;
}

static int celestia_getstar(lua_State* l)
{
    Celx_CheckArgs(l, 2, 2, "One argument expected to function celestia:getstar");

    CelestiaCore* appCore = this_celestia(l);
    double starIndex = Celx_SafeGetNumber(l, 2, AllErrors, "First arg to celestia:getstar must be a number");
    Universe* u = appCore->getSimulation()->getUniverse();
    Star* star = u->getStarCatalog()->getStar((uint32) starIndex);
    if (star == NULL)
        lua_pushnil(l);
    else
        object_new(l, Selection(star));

    return 1;
}

static int celestia_getdso(lua_State* l)
{
    Celx_CheckArgs(l, 2, 2, "One argument expected to function celestia:getdso");

    CelestiaCore* appCore = this_celestia(l);
    double dsoIndex = Celx_SafeGetNumber(l, 2, AllErrors, "First arg to celestia:getdso must be a number");
    Universe* u = appCore->getSimulation()->getUniverse();
    DeepSkyObject* dso = u->getDSOCatalog()->getDSO((uint32) dsoIndex);
    if (dso == NULL)
        lua_pushnil(l);
    else
        object_new(l, Selection(dso));

    return 1;
}


static int celestia_newvector(lua_State* l)
{
    Celx_CheckArgs(l, 4, 4, "Expected 3 arguments for celestia:newvector");
    // for error checking only:
    this_celestia(l);
    double x = Celx_SafeGetNumber(l, 2, AllErrors, "First arg to celestia:newvector must be a number");
    double y = Celx_SafeGetNumber(l, 3, AllErrors, "Second arg to celestia:newvector must be a number");
    double z = Celx_SafeGetNumber(l, 4, AllErrors, "Third arg to celestia:newvector must be a number");

    vector_new(l, Vec3d(x,y,z));

    return 1;
}

static int celestia_newposition(lua_State* l)
{
    Celx_CheckArgs(l, 4, 4, "Expected 3 arguments for celestia:newposition");
    // for error checking only:
    this_celestia(l);
    BigFix components[3];
    for (int i = 0; i < 3; i++)
    {
        if (lua_isnumber(l, i+2))
        {
            double v = lua_tonumber(l, i+2);
            components[i] = BigFix(v);
        }
        else if (lua_isstring(l, i+2))
        {
            components[i] = BigFix(string(lua_tostring(l, i+2)));
        }
        else
        {
            Celx_DoError(l, "Arguments to celestia:newposition must be either numbers or strings");
        }
    }

    position_new(l, UniversalCoord(components[0], components[1], components[2]));

    return 1;
}

static int celestia_newrotation(lua_State* l)
{
    Celx_CheckArgs(l, 3, 5, "Need 2 or 4 arguments for celestia:newrotation");
    // for error checking only:
    this_celestia(l);

    if (lua_gettop(l) > 3)
    {
        // if (lua_gettop == 4), Celx_SafeGetNumber will catch the error
        double w = Celx_SafeGetNumber(l, 2, AllErrors, "arguments to celestia:newrotation must either be (vec, number) or four numbers");
        double x = Celx_SafeGetNumber(l, 3, AllErrors, "arguments to celestia:newrotation must either be (vec, number) or four numbers");
        double y = Celx_SafeGetNumber(l, 4, AllErrors, "arguments to celestia:newrotation must either be (vec, number) or four numbers");
        double z = Celx_SafeGetNumber(l, 5, AllErrors, "arguments to celestia:newrotation must either be (vec, number) or four numbers");
        Quatd q(w, x, y, z);
        rotation_new(l, q);
    }
    else
    {
        Vec3d* v = to_vector(l, 2);
        if (v == NULL)
        {
            Celx_DoError(l, "newrotation: first argument must be a vector");
        }
        double angle = Celx_SafeGetNumber(l, 3, AllErrors, "second argument to celestia:newrotation must be a number");
        Quatd q;
        q.setAxisAngle(*v, angle);
        rotation_new(l, q);
    }
    return 1;
}

static int celestia_getscripttime(lua_State* l)
{
    Celx_CheckArgs(l, 1, 1, "No arguments expected for celestia:getscripttime");
    // for error checking only:
    this_celestia(l);

    LuaState* luastate_ptr = getLuaStateObject(l);
    lua_pushnumber(l, luastate_ptr->getTime());
    return 1;
}

static int celestia_newframe(lua_State* l)
{
    Celx_CheckArgs(l, 2, 4, "One to three arguments expected for function celestia:newframe");
    int argc = lua_gettop(l);

    // for error checking only:
    this_celestia(l);

    const char* coordsysName = Celx_SafeGetString(l, 2, AllErrors, "newframe: first argument must be a string");
    ObserverFrame::CoordinateSystem coordSys = parseCoordSys(coordsysName);
    Selection* ref = NULL;
    Selection* target = NULL;

    if (coordSys == ObserverFrame::Universal)
    {
        frame_new(l, ObserverFrame());
    }
    else if (coordSys == ObserverFrame::PhaseLock)
    {
        if (argc >= 4)
        {
            ref = to_object(l, 3);
            target = to_object(l, 4);
        }

        if (ref == NULL || target == NULL)
        {
            Celx_DoError(l, "newframe: two objects required for lock frame");
        }

        frame_new(l, ObserverFrame(coordSys, *ref, *target));
    }
    else
    {
        if (argc >= 3)
            ref = to_object(l, 3);
        if (ref == NULL)
        {
            Celx_DoError(l, "newframe: one object argument required for frame");
        }

        frame_new(l, ObserverFrame(coordSys, *ref));
    }

    return 1;
}

static int celestia_requestkeyboard(lua_State* l)
{
    Celx_CheckArgs(l, 2, 2, "Need one arguments for celestia:requestkeyboard");
    CelestiaCore* appCore = this_celestia(l);

    if (!lua_isboolean(l, 2))
    {
        Celx_DoError(l, "First argument for celestia:requestkeyboard must be a boolean");
    }

    int mode = appCore->getTextEnterMode();

    if (lua_toboolean(l, 2))
    {
        // Check for existence of charEntered:
        lua_pushstring(l, KbdCallback);
        lua_gettable(l, LUA_GLOBALSINDEX);
        if (lua_isnil(l, -1))
        {
            Celx_DoError(l, "script requested keyboard, but did not provide callback");
        }
        lua_remove(l, -1);

        mode = mode | CelestiaCore::KbPassToScript;
    }
    else
    {
        mode = mode & ~CelestiaCore::KbPassToScript;
    }
    appCore->setTextEnterMode(mode);

    return 0;
}

static int celestia_registereventhandler(lua_State* l)
{
    Celx_CheckArgs(l, 3, 3, "Two arguments required for celestia:registereventhandler");
    //CelestiaCore* appCore = this_celestia(l);

    if (!lua_isstring(l, 2))
    {
        Celx_DoError(l, "First argument for celestia:registereventhandler must be a string");
    }

    if (!lua_isfunction(l, 3) && !lua_isnil(l, 3))
    {
        Celx_DoError(l, "Second argument for celestia:registereventhandler must be a function or nil");
    }

    lua_pushstring(l, EventHandlers);
    lua_gettable(l, LUA_REGISTRYINDEX);
    if (lua_isnil(l, -1))
    {
        // This should never happen--the table should be created when a new Celestia Lua
        // state is initialized.
        Celx_DoError(l, "Event handler table not created");
    }

    lua_pushvalue(l, 2);
    lua_pushvalue(l, 3);

    lua_settable(l, -3);

    return 0;
}

static int celestia_geteventhandler(lua_State* l)
{
    Celx_CheckArgs(l, 2, 2, "One argument expected for celestia:registereventhandler");
    //CelestiaCore* appCore = this_celestia(l);

    if (!lua_isstring(l, 2))
    {
        Celx_DoError(l, "Argument to celestia:geteventhandler must be a string");
    }

    lua_pushstring(l, EventHandlers);
    lua_gettable(l, LUA_REGISTRYINDEX);
    if (lua_isnil(l, -1))
    {
        // This should never happen--the table should be created when a new Celestia Lua
        // state is initialized.
        Celx_DoError(l, "Event handler table not created");
    }

    lua_pushvalue(l, 2);
    lua_gettable(l, -2);

    return 1;
}

static int celestia_takescreenshot(lua_State* l)
{
    Celx_CheckArgs(l, 1, 3, "Need 0 to 2 arguments for celestia:takescreenshot");
    CelestiaCore* appCore = this_celestia(l);
    LuaState* luastate = getLuaStateObject(l);
    // make sure we don't timeout because of taking a screenshot:
    double timeToTimeout = luastate->timeout - luastate->getTime();

    const char* filetype = Celx_SafeGetString(l, 2, WrongType, "First argument to celestia:takescreenshot must be a string");
    if (filetype == NULL)
        filetype = "png";

    // Let the script safely contribute one part of the filename:
    const char* fileid_ptr = Celx_SafeGetString(l, 3, WrongType, "Second argument to celestia:takescreenshot must be a string");
    if (fileid_ptr == NULL)
        fileid_ptr = "";
    string fileid(fileid_ptr);

    // be paranoid about the fileid, make sure it only contains 'A-Za-z0-9_':
    for (unsigned int i = 0; i < fileid.length(); i++)
    {
        char ch = fileid[i];
        if (!((ch >= 'a' && ch <= 'z') ||
              (fileid[i] >= 'A' && ch <= 'Z') ||
              (ch >= '0' && ch <= '9') ) )
            fileid[i] = '_';
    }
    // limit length of string
    if (fileid.length() > 16)
        fileid = fileid.substr(0, 16);
    if (fileid.length() > 0)
        fileid.append("-");

    string path = appCore->getConfig()->scriptScreenshotDirectory;
    if (path.length() > 0 &&
        path[path.length()-1] != '/' &&
        path[path.length()-1] != '\\')

        path.append("/");

    luastate->screenshotCount++;
    bool success = false;
    char filenamestem[48];
    sprintf(filenamestem, "screenshot-%s%06i", fileid.c_str(), luastate->screenshotCount);

    // Get the dimensions of the current viewport
    GLint viewport[4];
    glGetIntegerv(GL_VIEWPORT, viewport);

#ifndef TARGET_OS_MAC
    if (strncmp(filetype, "jpg", 3) == 0)
    {
        string filepath = path + filenamestem + ".jpg";
        success = CaptureGLBufferToJPEG(string(filepath),
                                       viewport[0], viewport[1],
                                       viewport[2], viewport[3]);
    }
    else
    {
        string filepath = path + filenamestem + ".png";
        success = CaptureGLBufferToPNG(string(filepath),
                                       viewport[0], viewport[1],
                                       viewport[2], viewport[3]);
    }
#endif
    lua_pushboolean(l, success);

    // no matter how long it really took, make it look like 0.1s to timeout check:
    luastate->timeout = luastate->getTime() + timeToTimeout - 0.1;
    return 1;
}

static int celestia_createcelscript(lua_State* l)
{
    Celx_CheckArgs(l, 2, 2, "Need one argument for celestia:createcelscript()");
    string scripttext = Celx_SafeGetString(l, 2, AllErrors, "Argument to celestia:createcelscript() must be a string");
    return celscript_from_string(l, scripttext);
}

static int celestia_requestsystemaccess(lua_State* l)
{
    // ignore possible argument for future extensions
    Celx_CheckArgs(l, 1, 2, "No argument expected for celestia:requestsystemaccess()");
    this_celestia(l);
    LuaState* luastate = getLuaStateObject(l);
    luastate->requestIO();
    return 0;
}

static int celestia_getscriptpath(lua_State* l)
{
    // ignore possible argument for future extensions
    Celx_CheckArgs(l, 1, 1, "No argument expected for celestia:requestsystemaccess()");
    this_celestia(l);
    lua_pushstring(l, "celestia-scriptpath");
    lua_gettable(l, LUA_REGISTRYINDEX);
    return 1;
}

static int celestia_runscript(lua_State* l)
{
    Celx_CheckArgs(l, 2, 2, "One argument expected for celestia:runscript");
    string scriptfile = Celx_SafeGetString(l, 2, AllErrors, "Argument to celestia:runscript must be a string");

    lua_Debug ar;
    lua_getstack(l, 1, &ar);
    lua_getinfo(l, "S", &ar);
    string base_dir = ar.source; // Script file from which we are called
    if (base_dir[0] == '@') base_dir = base_dir.substr(1);
#ifdef _WIN32
    // Replace all backslashes with forward in base dir path
    size_t pos = base_dir.find('\\');
    while (pos != string::npos)
    {
        base_dir.replace(pos, 1, "/");
        pos = base_dir.find('\\');
    }
#endif
    // Remove script filename from path
    base_dir = base_dir.substr(0, base_dir.rfind('/')) + '/';

    CelestiaCore* appCore = this_celestia(l);
    appCore->runScript(base_dir + scriptfile);
    return 0;
}

static int celestia_tostring(lua_State* l)
{
    lua_pushstring(l, "[Celestia]");

    return 1;
}

static int celestia_windowbordersvisible(lua_State* l)
{
    Celx_CheckArgs(l, 1, 1, "No argument expected for celestia:windowbordersvisible");
    CelestiaCore* appCore = this_celestia(l);

    lua_pushboolean(l, appCore->getFramesVisible());

    return 1;
}

static int celestia_setwindowbordersvisible(lua_State* l)
{
    Celx_CheckArgs(l, 2, 2, "One argument expected for celestia:windowbordersvisible");
    CelestiaCore* appCore = this_celestia(l);

    bool visible = Celx_SafeGetBoolean(l, 2, AllErrors, "Argument to celestia:setwindowbordersvisible must be a boolean", true);
    appCore->setFramesVisible(visible);

    return 0;
}

static int celestia_seturl(lua_State* l)
{
    Celx_CheckArgs(l, 2, 3, "One or two arguments expected for celestia:seturl");
    CelestiaCore* appCore = this_celestia(l);

    string url = Celx_SafeGetString(l, 2, AllErrors, "First argument to celestia:seturl must be a string");
    Observer* obs = to_observer(l, 3);
    if (obs == NULL)
        obs = appCore->getSimulation()->getActiveObserver();
    View* view = getViewByObserver(appCore, obs);
    appCore->setActiveView(view);

    appCore->goToUrl(url);

    return 0;
}

static int celestia_geturl(lua_State* l)
{
    Celx_CheckArgs(l, 1, 2, "None or one argument expected for celestia:geturl");
    CelestiaCore* appCore = this_celestia(l);

    Observer* obs = to_observer(l, 2);
    if (obs == NULL)
        obs = appCore->getSimulation()->getActiveObserver();
    View* view = getViewByObserver(appCore, obs);
    appCore->setActiveView(view);

    CelestiaState appState;
    appState.captureState(appCore);

    Url url(appState, 3);
    lua_pushstring(l, url.getAsString().c_str());

    return 1;
}


static void CreateCelestiaMetaTable(lua_State* l)
{
    Celx_CreateClassMetatable(l, Celx_Celestia);

    Celx_RegisterMethod(l, "__tostring", celestia_tostring);
    Celx_RegisterMethod(l, "flash", celestia_flash);
    Celx_RegisterMethod(l, "print", celestia_print);
    Celx_RegisterMethod(l, "gettextwidth", celestia_gettextwidth);
    Celx_RegisterMethod(l, "show", celestia_show);
    Celx_RegisterMethod(l, "setaltazimuthmode", celestia_setaltazimuthmode);
    Celx_RegisterMethod(l, "getaltazimuthmode", celestia_getaltazimuthmode);
    Celx_RegisterMethod(l, "hide", celestia_hide);
    Celx_RegisterMethod(l, "getrenderflags", celestia_getrenderflags);
    Celx_RegisterMethod(l, "setrenderflags", celestia_setrenderflags);
    Celx_RegisterMethod(l, "getscreendimension", celestia_getscreendimension);
    Celx_RegisterMethod(l, "showlabel", celestia_showlabel);
    Celx_RegisterMethod(l, "hidelabel", celestia_hidelabel);
    Celx_RegisterMethod(l, "getlabelflags", celestia_getlabelflags);
    Celx_RegisterMethod(l, "setlabelflags", celestia_setlabelflags);
    Celx_RegisterMethod(l, "getorbitflags", celestia_getorbitflags);
    Celx_RegisterMethod(l, "setorbitflags", celestia_setorbitflags);
    Celx_RegisterMethod(l, "showconstellations", celestia_showconstellations);
    Celx_RegisterMethod(l, "hideconstellations", celestia_hideconstellations);
    Celx_RegisterMethod(l, "setconstellationcolor", celestia_setconstellationcolor);
    Celx_RegisterMethod(l, "setlabelcolor", celestia_setlabelcolor);
    Celx_RegisterMethod(l, "getlabelcolor", celestia_getlabelcolor);
    Celx_RegisterMethod(l, "setlinecolor",  celestia_setlinecolor);
    Celx_RegisterMethod(l, "getlinecolor",  celestia_getlinecolor);
    Celx_RegisterMethod(l, "settextcolor",  celestia_settextcolor);
    Celx_RegisterMethod(l, "gettextcolor",  celestia_gettextcolor);
    Celx_RegisterMethod(l, "getoverlayelements", celestia_getoverlayelements);
    Celx_RegisterMethod(l, "setoverlayelements", celestia_setoverlayelements);
    Celx_RegisterMethod(l, "getfaintestvisible", celestia_getfaintestvisible);
    Celx_RegisterMethod(l, "setfaintestvisible", celestia_setfaintestvisible);
    Celx_RegisterMethod(l, "getgalaxylightgain", celestia_getgalaxylightgain);
    Celx_RegisterMethod(l, "setgalaxylightgain", celestia_setgalaxylightgain);
    Celx_RegisterMethod(l, "setminfeaturesize", celestia_setminfeaturesize);
    Celx_RegisterMethod(l, "getminfeaturesize", celestia_getminfeaturesize);
    Celx_RegisterMethod(l, "getobserver", celestia_getobserver);
    Celx_RegisterMethod(l, "getobservers", celestia_getobservers);
    Celx_RegisterMethod(l, "getselection", celestia_getselection);
    Celx_RegisterMethod(l, "find", celestia_find);
    Celx_RegisterMethod(l, "select", celestia_select);
    Celx_RegisterMethod(l, "mark", celestia_mark);
    Celx_RegisterMethod(l, "unmark", celestia_unmark);
    Celx_RegisterMethod(l, "unmarkall", celestia_unmarkall);
    Celx_RegisterMethod(l, "gettime", celestia_gettime);
    Celx_RegisterMethod(l, "settime", celestia_settime);
    Celx_RegisterMethod(l, "ispaused", celestia_ispaused);
    Celx_RegisterMethod(l, "synchronizetime", celestia_synchronizetime);
    Celx_RegisterMethod(l, "istimesynchronized", celestia_istimesynchronized);
    Celx_RegisterMethod(l, "gettimescale", celestia_gettimescale);
    Celx_RegisterMethod(l, "settimescale", celestia_settimescale);
    Celx_RegisterMethod(l, "getambient", celestia_getambient);
    Celx_RegisterMethod(l, "setambient", celestia_setambient);
    Celx_RegisterMethod(l, "getminorbitsize", celestia_getminorbitsize);
    Celx_RegisterMethod(l, "setminorbitsize", celestia_setminorbitsize);
    Celx_RegisterMethod(l, "getstardistancelimit", celestia_getstardistancelimit);
    Celx_RegisterMethod(l, "setstardistancelimit", celestia_setstardistancelimit);
    Celx_RegisterMethod(l, "getstarstyle", celestia_getstarstyle);
    Celx_RegisterMethod(l, "setstarstyle", celestia_setstarstyle);
    Celx_RegisterMethod(l, "gettextureresolution", celestia_gettextureresolution);
    Celx_RegisterMethod(l, "settextureresolution", celestia_settextureresolution);
    Celx_RegisterMethod(l, "tojulianday", celestia_tojulianday);
    Celx_RegisterMethod(l, "fromjulianday", celestia_fromjulianday);
    Celx_RegisterMethod(l, "utctotdb", celestia_utctotdb);
    Celx_RegisterMethod(l, "tdbtoutc", celestia_tdbtoutc);
    Celx_RegisterMethod(l, "getsystemtime", celestia_getsystemtime);
    Celx_RegisterMethod(l, "getstarcount", celestia_getstarcount);
    Celx_RegisterMethod(l, "getdsocount", celestia_getdsocount);
    Celx_RegisterMethod(l, "getstar", celestia_getstar);
    Celx_RegisterMethod(l, "getdso", celestia_getdso);
    Celx_RegisterMethod(l, "newframe", celestia_newframe);
    Celx_RegisterMethod(l, "newvector", celestia_newvector);
    Celx_RegisterMethod(l, "newposition", celestia_newposition);
    Celx_RegisterMethod(l, "newrotation", celestia_newrotation);
    Celx_RegisterMethod(l, "getscripttime", celestia_getscripttime);
    Celx_RegisterMethod(l, "requestkeyboard", celestia_requestkeyboard);
    Celx_RegisterMethod(l, "takescreenshot", celestia_takescreenshot);
    Celx_RegisterMethod(l, "createcelscript", celestia_createcelscript);
    Celx_RegisterMethod(l, "requestsystemaccess", celestia_requestsystemaccess);
    Celx_RegisterMethod(l, "getscriptpath", celestia_getscriptpath);
    Celx_RegisterMethod(l, "runscript", celestia_runscript);
    Celx_RegisterMethod(l, "registereventhandler", celestia_registereventhandler);
    Celx_RegisterMethod(l, "geteventhandler", celestia_geteventhandler);
    Celx_RegisterMethod(l, "stars", celestia_stars);
    Celx_RegisterMethod(l, "dsos", celestia_dsos);
    Celx_RegisterMethod(l, "windowbordersvisible", celestia_windowbordersvisible);
    Celx_RegisterMethod(l, "setwindowbordersvisible", celestia_setwindowbordersvisible);
    Celx_RegisterMethod(l, "seturl", celestia_seturl);
    Celx_RegisterMethod(l, "geturl", celestia_geturl);

    lua_pop(l, 1);
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

    lua_pushstring(state, "KM_PER_MICROLY");
    lua_pushnumber(state, (lua_Number)KM_PER_LY/1e6);
    lua_settable(state, LUA_GLOBALSINDEX);

    loadLuaLibs(state);

    // Create the celestia object
    lua_pushstring(state, "celestia");
    celestia_new(state, appCore);
    lua_settable(state, LUA_GLOBALSINDEX);
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
    lua_pushstring(state, "dofile");
    lua_gettable(state, LUA_GLOBALSINDEX); // function "dofile" on stack
    lua_pushstring(state, "luainit.celx"); // parameter
    if (lua_pcall(state, 1, 0, 0) != 0) // execute it
    {
        CelestiaCore::Alerter* alerter = appCore->getAlerter();
        // copy string?!
        const char* errorMessage = lua_tostring(state, -1);
        cout << errorMessage << '\n'; cout.flush();
        alerter->fatalError(errorMessage);
        return false;
    }
#endif

    return true;
}


void LuaState::setLuaPath(const string& s)
{
#if LUA_VER >= 0x050100
    lua_getfield(state, LUA_GLOBALSINDEX, "package");
    lua_pushstring(state, s.c_str());
    lua_setfield(state, -2, "path");
    lua_pop(state, 1);
#else
    lua_pushstring(state, "LUA_PATH");
    lua_pushstring(state, s.c_str());
    lua_settable(state, LUA_GLOBALSINDEX);
#endif
}


// ==================== Font Object ====================

static int font_new(lua_State* l, TextureFont* f)
{
    TextureFont** ud = static_cast<TextureFont**>(lua_newuserdata(l, sizeof(TextureFont*)));
    *ud = f;

    Celx_SetClass(l, Celx_Font);

    return 1;
}

static TextureFont* to_font(lua_State* l, int index)
{
    TextureFont** f = static_cast<TextureFont**>(lua_touserdata(l, index));

    // Check if pointer is valid
    if (f != NULL )
    {
            return *f;
    }
    return NULL;
}

static TextureFont* this_font(lua_State* l)
{
    TextureFont* f = to_font(l, 1);
    if (f == NULL)
    {
        Celx_DoError(l, "Bad font object!");
    }

    return f;
}


static int font_bind(lua_State* l)
{
    Celx_CheckArgs(l, 1, 1, "No arguments expected for font:bind()");

    TextureFont* font = this_font(l);
    font->bind();
    return 0;
}

static int font_render(lua_State* l)
{
    Celx_CheckArgs(l, 2, 2, "One argument required for font:render");

    const char* s = Celx_SafeGetString(l, 2, AllErrors, "First argument to font:render must be a string");
    TextureFont* font = this_font(l);
	font->render(s);

    return 0;
}

static int font_getwidth(lua_State* l)
{
    Celx_CheckArgs(l, 2, 2, "One argument expected for font:getwidth");
    const char* s = Celx_SafeGetString(l, 2, AllErrors, "Argument to font:getwidth must be a string");
    TextureFont* font = this_font(l);
    lua_pushnumber(l, font->getWidth(s));
    return 1;
}

static int font_getheight(lua_State* l)
{
    Celx_CheckArgs(l, 1, 1, "No arguments expected for font:getheight()");

    TextureFont* font = this_font(l);
    lua_pushnumber(l, font->getHeight());
    return 1;
}

static int font_tostring(lua_State* l)
{
    // TODO: print out the actual information about the font
    lua_pushstring(l, "[Font]");

    return 1;
}

static void CreateFontMetaTable(lua_State* l)
{
    Celx_CreateClassMetatable(l, Celx_Font);

    Celx_RegisterMethod(l, "__tostring", font_tostring);
    Celx_RegisterMethod(l, "bind", font_bind);
    Celx_RegisterMethod(l, "render", font_render);
    Celx_RegisterMethod(l, "getwidth", font_getwidth);
    Celx_RegisterMethod(l, "getheight", font_getheight);

    lua_pop(l, 1); // remove metatable from stack
}

// ==================== Image =============================================
#if 0
static int image_new(lua_State* l, Image* i)
{
    Image** ud = static_cast<Image**>(lua_newuserdata(l, sizeof(Image*)));
    *ud = i;

    Celx_SetClass(l, Celx_Image);

    return 1;
}
#endif

static Image* to_image(lua_State* l, int index)
{
    Image** image = static_cast<Image**>(lua_touserdata(l, index));

    // Check if pointer is valid
    if (image != NULL )
    {
            return *image;
    }
    return NULL;
}

static Image* this_image(lua_State* l)
{
    Image* image = to_image(l,1);
    if (image == NULL)
    {
        Celx_DoError(l, "Bad image object!");
    }

    return image;
}

static int image_getheight(lua_State* l)
{
    Celx_CheckArgs(l, 1, 1, "No arguments expected for image:getheight()");

    Image* image = this_image(l);
    lua_pushnumber(l, image->getHeight());
    return 1;
}

static int image_getwidth(lua_State* l)
{
    Celx_CheckArgs(l, 1, 1, "No arguments expected for image:getwidth()");

    Image* image = this_image(l);
    lua_pushnumber(l, image->getWidth());
    return 1;
}

static int image_tostring(lua_State* l)
{
    // TODO: print out the actual information about the image
    lua_pushstring(l, "[Image]");

    return 1;
}

static void CreateImageMetaTable(lua_State* l)
{
    Celx_CreateClassMetatable(l, Celx_Image);

    Celx_RegisterMethod(l, "__tostring", image_tostring);
    Celx_RegisterMethod(l, "getheight", image_getheight);
    Celx_RegisterMethod(l, "getwidth", image_getwidth);

    lua_pop(l, 1); // remove metatable from stack
}

// ==================== Texture ============================================

static int texture_new(lua_State* l, Texture* t)
{
    Texture** ud = static_cast<Texture**>(lua_newuserdata(l, sizeof(Texture*)));
    *ud = t;

    Celx_SetClass(l, Celx_Texture);

    return 1;
}

static Texture* to_texture(lua_State* l, int index)
{
    Texture** texture = static_cast<Texture**>(lua_touserdata(l, index));

    // Check if pointer is valid
    if (texture != NULL )
    {
            return *texture;
    }
    return NULL;
}

static Texture* this_texture(lua_State* l)
{
    Texture* texture = to_texture(l,1);
    if (texture == NULL)
    {
        Celx_DoError(l, "Bad texture object!");
    }

    return texture;
}

static int texture_bind(lua_State* l)
{
    Celx_CheckArgs(l, 1, 1, "No arguments expected for texture:bind()");

    Texture* texture = this_texture(l);
    texture->bind();
    return 0;
}

static int texture_getheight(lua_State* l)
{
    Celx_CheckArgs(l, 1, 1, "No arguments expected for texture:getheight()");

    Texture* texture = this_texture(l);
    lua_pushnumber(l, texture->getHeight());
    return 1;
}

static int texture_getwidth(lua_State* l)
{
    Celx_CheckArgs(l, 1, 1, "No arguments expected for texture:getwidth()");

    Texture* texture = this_texture(l);
    lua_pushnumber(l, texture->getWidth());
    return 1;
}

static int texture_tostring(lua_State* l)
{
    // TODO: print out the actual information about the texture
    lua_pushstring(l, "[Texture]");

    return 1;
}

static void CreateTextureMetaTable(lua_State* l)
{
    Celx_CreateClassMetatable(l, Celx_Texture);

    Celx_RegisterMethod(l, "__tostring", texture_tostring);
    Celx_RegisterMethod(l, "getheight", texture_getheight);
    Celx_RegisterMethod(l, "getwidth", texture_getwidth);
    Celx_RegisterMethod(l, "bind", texture_bind);

    lua_pop(l, 1); // remove metatable from stack
}

// ==================== celestia extensions ====================

static int celestia_log(lua_State* l)
{
    Celx_CheckArgs(l, 2, 2, "One argument expected to function celestia:log");

    const char* s = Celx_SafeGetString(l, 2, AllErrors, "First argument to celestia:log must be a string");
	clog << s << "\n"; clog.flush();
	return 0;
}

static int celestia_getparamstring(lua_State* l)
{
    Celx_CheckArgs(l, 2, 2, "One argument expected to celestia:getparamstring()");
    CelestiaCore* appCore = this_celestia(l);
    const char* s = Celx_SafeGetString(l, 2, AllErrors, "Argument to celestia:getparamstring must be a string");
    std::string paramString; // HWR
    CelestiaConfig* config = appCore->getConfig();
    config->configParams->getString(s, paramString);
    lua_pushstring(l,paramString.c_str());
    return 1;
}

static int celestia_loadtexture(lua_State* l)
{
    Celx_CheckArgs(l, 2, 2, "Need one argument for celestia:loadtexture()");
    string s = Celx_SafeGetString(l, 2, AllErrors, "Argument to celestia:loadtexture() must be a string");
    lua_Debug ar;
    lua_getstack(l, 1, &ar);
    lua_getinfo(l, "S", &ar);
    string base_dir = ar.source; // Lua file from which we are called
    if (base_dir[0] == '@') base_dir = base_dir.substr(1);
    base_dir = base_dir.substr(0, base_dir.rfind('/')) + '/';
    Texture* t = LoadTextureFromFile(base_dir + s);
    if (t == NULL) return 0;
    texture_new(l, t);
    return 1;
}

static int celestia_loadfont(lua_State* l)
{
    Celx_CheckArgs(l, 2, 2, "Need one argument for celestia:loadtexture()");
    string s = Celx_SafeGetString(l, 2, AllErrors, "Argument to celestia:loadfont() must be a string");
    TextureFont* font = LoadTextureFont(s);
    if (font == NULL) return 0;
	font->buildTexture();
	font_new(l, font);
    return 1;
}

TextureFont* getFont(CelestiaCore* appCore)
{
       return appCore->font;
}

static int celestia_getfont(lua_State* l)
{
    Celx_CheckArgs(l, 1, 1, "No arguments expected to function celestia:getTitleFont");

    CelestiaCore* appCore = getAppCore(l, AllErrors);
       TextureFont* font = getFont(appCore);
    if (font == NULL) return 0;
       font_new(l, font);
    return 1;
}

TextureFont* getTitleFont(CelestiaCore* appCore)
{
	return appCore->titleFont;
}

static int celestia_gettitlefont(lua_State* l)
{
    Celx_CheckArgs(l, 1, 1, "No arguments expected to function celestia:getTitleFont");

    CelestiaCore* appCore = getAppCore(l, AllErrors);
	TextureFont* font = getTitleFont(appCore);
    if (font == NULL) return 0;
	font_new(l, font);
    return 1;
}

static int celestia_settimeslice(lua_State* l)
{
    Celx_CheckArgs(l, 2, 2, "One argument required for celestia:settimeslice");
    //CelestiaCore* appCore = this_celestia(l);

    if (!lua_isnumber(l, 2) && !lua_isnil(l, 2))
    {
        Celx_DoError(l, "Argument for celestia:settimeslice must be a number");
    }
    double timeslice = Celx_SafeGetNumber(l, 2, AllErrors, "Argument to celestia:settimeslice must be a number");
    if (timeslice == 0.0)
        timeslice = 0.1;

    LuaState* luastate = getLuaStateObject(l);
    luastate->timeout = luastate->getTime() + timeslice;

    return 0;
}

static int celestia_setluahook(lua_State* l)
{
    Celx_CheckArgs(l, 2, 2, "One argument required for celestia:setluahook");
    CelestiaCore* appCore = this_celestia(l);

    if (!lua_istable(l, 2) && !lua_isnil(l, 2))
    {
        Celx_DoError(l, "Argument for celestia:setluahook must be a table or nil");
        return 0;
    }

    LuaState* luastate = getLuaStateObject(l);
    if (luastate != NULL)
    {
        luastate->setLuaHookEventHandlerEnabled(lua_istable(l, 2));
    }

    lua_pushlightuserdata(l, appCore);
    lua_pushvalue(l, -2);
    lua_settable(l, LUA_REGISTRYINDEX);

    return 0;
}

static void ExtendCelestiaMetaTable(lua_State* l)
{
    PushClass(l, Celx_Celestia);
    lua_rawget(l, LUA_REGISTRYINDEX);
    if (lua_type(l, -1) != LUA_TTABLE)
        cout << "Metatable for " << CelxLua::ClassNames[Celx_Celestia] << " not found!\n";
    Celx_RegisterMethod(l, "log", celestia_log);
    Celx_RegisterMethod(l, "settimeslice", celestia_settimeslice);
    Celx_RegisterMethod(l, "setluahook", celestia_setluahook);
    Celx_RegisterMethod(l, "getparamstring", celestia_getparamstring);
    Celx_RegisterMethod(l, "getfont", celestia_getfont);
    Celx_RegisterMethod(l, "gettitlefont", celestia_gettitlefont);
    Celx_RegisterMethod(l, "loadtexture", celestia_loadtexture);
    Celx_RegisterMethod(l, "loadfont", celestia_loadfont);
    lua_pop(l, 1);
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
 if (lib!=NULL)
 {
  lua_CFunction f=(lua_CFunction) dlsym(lib,init);
  if (f!=NULL)
  {
   lua_pushlightuserdata(L,lib);
   lua_pushcclosure(L,f,1);
   return 1;
  }
 }
 /* else return appropriate error messages */
 lua_pushnil(L);
 lua_pushstring(L,dlerror());
 lua_pushstring(L,(lib!=NULL) ? "init" : "open");
 if (lib!=NULL) dlclose(lib);
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
    lua_getfield(state, LUA_GLOBALSINDEX, "package");
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


CelxLua::~CelxLua()
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
