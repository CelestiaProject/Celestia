// celx.cpp
//
// Copyright (C) 2003, Chris Laurel <claurel@shatters.net>
//
// Lua script extensions for Celestia
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include <cassert>
#include <cstring>
#include <cstdio>
#include <map>
#include <celengine/astro.h>
#include <celengine/celestia.h>
#include <celengine/cmdparser.h>
#include <celengine/execenv.h>
#include <celengine/execution.h>
#include <celmath/vecmath.h>
#include <celengine/gl.h>
#include "imagecapture.h"

// Ugh . . . the C++ standard says that stringstream should be in
// sstream, but the GNU C++ compiler uses strstream instead.
#ifdef HAVE_SSTREAM
#include <sstream>
#else
#include <strstream>
#endif // HAVE_SSTREAM

#include "celx.h"
#include "celestiacore.h"
extern "C" {
#include "lualib.h"
}

using namespace std;

static const char* ClassNames[] =
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
};

static const int _Celestia = 0;
static const int _Observer = 1;
static const int _Object   = 2;
static const int _Vec3     = 3;
static const int _Matrix   = 4;
static const int _Rotation = 5;
static const int _Position = 6;
static const int _Frame    = 7;
static const int _CelScript= 8;

#define CLASS(i) ClassNames[(i)]

// Maximum timeslice a script may run without
// returning control to celestia
static const double MaxTimeslice = 5.0;

// names of callback-functions in Lua:
static const char* KbdCallback = "celestia_keyboard_callback";
static const char* CleanupCallback = "celestia_cleanup_callback";

typedef map<string, uint32> FlagMap; 

static FlagMap RenderFlagMap;
static FlagMap LabelFlagMap;
static FlagMap LocationFlagMap;
static FlagMap BodyTypeMap;
static bool mapsInitialized = false;

// select which type of error will be fatal (call lua_error) and
// which will return a default value instead
enum FatalErrors {
    NoErrors = 0,
    WrongType = 1,
    WrongArgc = 2,
    AllErrors = WrongType | WrongArgc,
};


// Initialize various maps from named keywords to numeric flags used within celestia:
static void initRenderFlagMap()
{
    RenderFlagMap["orbits"] = Renderer::ShowOrbits;
    RenderFlagMap["cloudmaps"] = Renderer::ShowCloudMaps;
    RenderFlagMap["constellations"] = Renderer::ShowDiagrams;
    RenderFlagMap["galaxies"] = Renderer::ShowGalaxies;
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
    RenderFlagMap["smoothlines"] = Renderer::ShowSmoothLines;
}

static void initLabelFlagMap()
{
    LabelFlagMap["planets"] = Renderer::PlanetLabels;
    LabelFlagMap["moons"] = Renderer::MoonLabels;
    LabelFlagMap["spacecraft"] = Renderer::SpacecraftLabels;
    LabelFlagMap["asteroids"] = Renderer::AsteroidLabels;
    LabelFlagMap["comets"] = Renderer::CometLabels;
    LabelFlagMap["constellations"] = Renderer::ConstellationLabels;
    LabelFlagMap["stars"] = Renderer::StarLabels;
    LabelFlagMap["galaxies"] = Renderer::GalaxyLabels;
    LabelFlagMap["locations"] = Renderer::LocationLabels;
}

static void initBodyTypeMap()
{
    BodyTypeMap["Planet"] = Body::Planet;
    BodyTypeMap["Moon"] = Body::Moon;
    BodyTypeMap["Asteroid"] = Body::Asteroid;
    BodyTypeMap["Comet"] = Body::Comet;
    BodyTypeMap["Spacecraft"] = Body::Spacecraft;
    BodyTypeMap["Invisible"] = Body::Invisible;
    BodyTypeMap["Unknown"] = Body::Unknown;
}

static void initLocationFlagMap()
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
    LocationFlagMap["astrum"] = Location::Astrum;
    LocationFlagMap["corona"] = Location::Corona;
    LocationFlagMap["dorsum"] = Location::Dorsum;
    LocationFlagMap["fossa"] = Location::Fossa;
    LocationFlagMap["catena"] = Location::Catena;
    LocationFlagMap["mensa"] = Location::Mensa;
    LocationFlagMap["rima"] = Location::Rima;
    LocationFlagMap["undae"] = Location::Undae;
    LocationFlagMap["reticulum"] = Location::Reticulum;
    LocationFlagMap["planitia"] = Location::Planitia;
    LocationFlagMap["linea"] = Location::Linea;
    LocationFlagMap["fluctus"] = Location::Fluctus;
    LocationFlagMap["farrum"] = Location::Farrum;
    LocationFlagMap["other"] = Location::Other;
}

static void initMaps()
{
    if (!mapsInitialized)
    {
        initRenderFlagMap();
        initLabelFlagMap();
        initBodyTypeMap();
        initLocationFlagMap();
    }
    mapsInitialized = true;
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
    lua_pushlstring(l, ClassNames[id], strlen(ClassNames[id]));
}

// Set the class (metatable) of the object on top of the stack
static void SetClass(lua_State* l, int id)
{
    PushClass(l, id);
    lua_rawget(l, LUA_REGISTRYINDEX);
    if (lua_type(l, -1) != LUA_TTABLE)
        cout << "Metatable for " << ClassNames[id] << " not found!\n";
    if (lua_setmetatable(l, -2) == 0)
        cout << "Error setting metatable for " << ClassNames[id] << '\n';
}

// Initialize the metatable for a class; sets the appropriate registry
// entries and __index, leaving the metatable on the stack when done.
static void CreateClassMetatable(lua_State* l, int id)
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
static void RegisterMethod(lua_State* l, const char* name, lua_CFunction fn)
{
    lua_pushstring(l, name);
    lua_pushvalue(l, -2);
    lua_pushcclosure(l, fn, 1);
    lua_settable(l, -3);
}


// Verify that an object at location index on the stack is of the
// specified class
static bool istype(lua_State* l, int index, int id)
{
    // get registry[metatable]
    if (!lua_getmetatable(l, index))
        return false;
    lua_rawget(l, LUA_REGISTRYINDEX);

    if (lua_type(l, -1) != LUA_TSTRING)
    {
        cout << "istype failed!  Unregistered class.\n";
        lua_pop(l, 1);
        return false;
    }

    const char* classname = lua_tostring(l, -1);
    if (classname != NULL && strcmp(classname, ClassNames[id]) == 0)
    {
        lua_pop(l, 1);
        return true;
    }
    lua_pop(l, 1);
    return false;
}

// Verify that an object at location index on the stack is of the
// specified class and return pointer to userdata
static void* CheckUserData(lua_State* l, int index, int id)
{
    if (istype(l, index, id))
        return lua_touserdata(l, index);
    else
        return NULL;
}


LuaState::LuaState() :
    timeout(MaxTimeslice),
    state(NULL),
    costate(NULL),
    alive(false),
    timer(NULL)
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

// Callback for CelestiaCore::charEntered.
// Returns true if keypress has been consumed 
bool LuaState::charEntered(const char* c_p)
{
    int stack_top = lua_gettop(costate);
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
            result = static_cast<bool>(lua_toboolean(costate, -1));
        }
    }
    // cleanup stack - is this necessary?
    lua_settop(costate, stack_top);
    return result;
}

// Check if the running script has exceeded its allowed timeslice
// and terminate it if it has:
static void checkTimeslice(lua_State *l, lua_Debug *ar)
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


static int auxresume(lua_State *L, lua_State *co, int narg)
{
    int status;
#if 0
    if (!lua_checkstack(co, narg))
        luaL_error(L, "too many arguments to resume");
#endif
    lua_xmove(L, co, narg);

    status = lua_resume(co, narg);
    if (status == 0)
    {
        int nres = lua_gettop(co);
#if 0
        if (!lua_checkstack(L, narg))
              luaL_error(L, "too many results to resume");
#endif
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

static const char* readStreamChunk(lua_State* state, void* udata, size_t* size)
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


// Return the CelestiaCore object stored in the globals table
static CelestiaCore* getAppCore(lua_State* l, FatalErrors fatalErrors = NoErrors)
{
    lua_pushstring(l, "celestia");
    lua_gettable(l, LUA_GLOBALSINDEX);
    CelestiaCore** appCore =
        static_cast<CelestiaCore**>(CheckUserData(l, -1, _Celestia));
    lua_pop(l, 1);

    if (appCore == NULL)
    {
        if (fatalErrors == NoErrors)
            return NULL;
        else
        {
            lua_pushstring(l, "internal error: invalid appCore");
            lua_error(l);
        }
    }

    return *appCore;
}


int LuaState::loadScript(istream& in, const string& streamname)
{
    char buf[4096];
    ReadChunkInfo info;
    info.buf = buf;
    info.bufSize = sizeof(buf);
    info.in = &in;

    int status = lua_load(state, readStreamChunk, &info, streamname.c_str());
    if (status != 0)
        cout << "Error loading script: " << lua_tostring(state, -1) << '\n';

    return status;
}

int LuaState::loadScript(const string& s)
{
#ifdef HAVE_SSTREAM
    istringstream in(s);
#else
    istrstream in(s.c_str());
#endif
    return loadScript(in, "string");
}

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
    int nArgs = auxresume(state, co, 0);
    if (nArgs < 0)
    {
        alive = false;

        const char* errorMessage = lua_tostring(state, -1);

        // This is a nasty and hopefully temporary hack . . .  We continue
        // to resume the script until we get an error.  The
        // 'cannot resume dead coroutine' error appears when there were
        // no other errors, and execution terminates normally.  There
        // should be a better way to figure out whether the script ended
        // normally . . .
        if (strcmp(errorMessage, "cannot resume dead coroutine") != 0)
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
        return nArgs; // arguments from yield
    }
}


// get current linenumber of script and create
// useful error-message
static void doError(lua_State* l, const char* errorMsg)
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


// Check if the number of arguments on the stack matches
// the allowed range [minArgs, maxArgs]. Cause an error if not. 
static void checkArgs(lua_State* l,
                      int minArgs, int maxArgs, const char* errorMessage)
{
    int argc = lua_gettop(l);
    if (argc < minArgs || argc > maxArgs)
    {
        doError(l, errorMessage);
    }
}


static astro::CoordinateSystem parseCoordSys(const string& name)
{
    if (compareIgnoringCase(name, "universal") == 0)
        return astro::Universal;
    else if (compareIgnoringCase(name, "ecliptic") == 0)
        return astro::Ecliptical;
    else if (compareIgnoringCase(name, "equatorial") == 0)
        return astro::Equatorial;
    else if (compareIgnoringCase(name, "planetographic") == 0)
        return astro::Geographic;
    else if (compareIgnoringCase(name, "observer") == 0)
        return astro::ObserverLocal;
    else if (compareIgnoringCase(name, "lock") == 0)
        return astro::PhaseLock;
    else if (compareIgnoringCase(name, "chase") == 0)
        return astro::Chase;
    else
        return astro::Universal;
}


static Marker::Symbol parseMarkerSymbol(const string& name)
{
    if (compareIgnoringCase(name, "diamond") == 0)
        return Marker::Diamond;
    else if (compareIgnoringCase(name, "triangle") == 0)
        return Marker::Triangle;
    else if (compareIgnoringCase(name, "square") == 0)
        return Marker::Square;
    else if (compareIgnoringCase(name, "plus") == 0)
        return Marker::Plus;
    else if (compareIgnoringCase(name, "x") == 0)
        return Marker::X;
    else
        return Marker::Diamond;
}


// Get a pointer to the LuaState-object from the registry:
LuaState* getLuaStateObject(lua_State* l)
{
    int stackSize = lua_gettop(l);
    lua_pushstring(l, "celestia-luastate");
    lua_gettable(l, LUA_REGISTRYINDEX);

    if (!lua_islightuserdata(l, -1))
    {
        doError(l, "Internal Error: Invalid table entry for LuaState-pointer");
    }
    LuaState* luastate_ptr = static_cast<LuaState*>(lua_touserdata(l, -1));
    if (luastate_ptr == NULL)
    {
        doError(l, "Internal Error: Invalid LuaState-pointer");
    }
    lua_settop(l, stackSize);
    return luastate_ptr;
}


// Map the observer to its View. Return NULL if no view exists 
// for this observer (anymore).
View* getViewByObserver(CelestiaCore* appCore, Observer* obs)
{
    for (unsigned int i = 0; i < appCore->views.size(); i++)
    {
        if ((appCore->views[i])->observer == obs)
        {
            return appCore->views[i];
        }
    }
    return NULL;
}

// Fill list with all Observers
void getObservers(CelestiaCore* appCore, std::vector<Observer*>& list)
{
    for (unsigned int i = 0; i < appCore->views.size(); i++)
    {
        list.push_back(appCore->views[i]->observer);
    }
}

// ==================== Helpers ====================

// safe wrapper for lua_tostring: fatal errors will terminate script by calling
// lua_error with errorMsg.
static const char* safeGetString(lua_State* l, int index, FatalErrors fatalErrors = AllErrors,
                                 const char* errorMsg = "String argument expected")
{
    if (l == NULL)
    {
        cerr << "Error: LuaState invalid in safeGetString\n";
        cout << "Error: LuaState invalid in safeGetString\n";
        return NULL;
    }
    int argc = lua_gettop(l);
    if (index < 1 || index > argc)
    {
        if (fatalErrors & WrongArgc)
        {
            doError(l, errorMsg);
        }
        else
            return NULL;
    }
    if (!lua_isstring(l, index))
    {
        if (fatalErrors & WrongType)
        {
            doError(l, errorMsg);
        }
        else
            return NULL;
    }
    return lua_tostring(l, index);
}

// safe wrapper for lua_tonumber, c.f. safeGetString
// Non-fatal errors will return  defaultValue.
static lua_Number safeGetNumber(lua_State* l, int index, FatalErrors fatalErrors = AllErrors,
                                 const char* errorMsg = "Numeric argument expected",
                                 lua_Number defaultValue = 0.0)
{
    if (l == NULL)
    {
        cerr << "Error: LuaState invalid in safeGetNumber\n";
        cout << "Error: LuaState invalid in safeGetNumber\n";
        return 0.0;
    }
    int argc = lua_gettop(l);
    if (index < 1 || index > argc)
    {
        if (fatalErrors & WrongArgc)
        {
            doError(l, errorMsg);
        }
        else
            return defaultValue;
    }
    if (!lua_isnumber(l, index))
    {
        if (fatalErrors & WrongType)
        {
            doError(l, errorMsg);
        }
        else
            return defaultValue;
    }
    return lua_tonumber(l, index);
}


// Add a field to the table on top of the stack
static void setTable(lua_State* l, const char* field, lua_Number value)
{
    lua_pushstring(l, field);
    lua_pushnumber(l, value);
    lua_settable(l, -3);
}

static void setTable(lua_State* l, const char* field, const char* value)
{
    lua_pushstring(l, field);
    lua_pushstring(l, value);
    lua_settable(l, -3);
}

// ==================== forward declarations ====================

// must be declared for vector_add:
static UniversalCoord* to_position(lua_State* l, int index);
static int position_new(lua_State* l, const UniversalCoord& uc);
// for frame_getrefobject / gettargetobject
static int object_new(lua_State* l, const Selection& sel);

// ==================== Vector ====================
static int vector_new(lua_State* l, const Vec3d& v)
{
    Vec3d* v3 = reinterpret_cast<Vec3d*>(lua_newuserdata(l, sizeof(Vec3d)));
    *v3 = v;
    SetClass(l, _Vec3);

    return 1;
}

static Vec3d* to_vector(lua_State* l, int index)
{
    return static_cast<Vec3d*>(CheckUserData(l, index, _Vec3));
}

static Vec3d* this_vector(lua_State* l)
{
    Vec3d* v3 = to_vector(l, 1);
    if (v3 == NULL)
    {
        doError(l, "Bad vector object!");
    }

    return v3;
}


static int vector_sub(lua_State* l)
{
    checkArgs(l, 2, 2, "Need two operands for sub");
    Vec3d* op1 = to_vector(l, 1);
    Vec3d* op2 = to_vector(l, 2);
    if (op1 == NULL || op2 == NULL)
    {
        doError(l, "Subtraction only defined for two vectors");
    }
    else
    {
        Vec3d result = *op1 - *op2;
        vector_new(l, result);
    }
    return 1;
}

static int vector_get(lua_State* l)
{
    checkArgs(l, 2, 2, "Invalid access of vector-component");
    Vec3d* v3 = this_vector(l);
    string key = safeGetString(l, 2, AllErrors, "Invalid key in vector-access");
    double value = 0.0;
    if (key == "x")
        value = v3->x;
    else if (key == "y")
        value = v3->y;
    else if (key == "z")
        value = v3->z;
    else
    {
        if (lua_getmetatable(l, 1))
        {
            lua_pushvalue(l, 2);
            lua_rawget(l, -2);
            return 1;
        }
        else
        {
            doError(l, "Internal error: couldn't get metatable");
        }
    }
    lua_pushnumber(l, (lua_Number)value);
    return 1;
}

static int vector_set(lua_State* l)
{
    checkArgs(l, 3, 3, "Invalid access of vector-component");
    Vec3d* v3 = this_vector(l);
    string key = safeGetString(l, 2, AllErrors, "Invalid key in vector-access");
    double value = safeGetNumber(l, 3, AllErrors, "Vector components must be numbers");
    if (key == "x")
        v3->x = value;
    else if (key == "y")
        v3->y = value;
    else if (key == "z")
        v3->z = value;
    else
    {
        doError(l, "Invalid key in vector-access");
    }
    return 0;
}

static int vector_getx(lua_State* l)
{
    checkArgs(l, 1, 1, "No arguments expected for vector:getx");
    Vec3d* v3 = this_vector(l);
    lua_Number x;
    x = static_cast<lua_Number>(v3->x);
    lua_pushnumber(l, x);

    return 1;
}

static int vector_gety(lua_State* l)
{
    checkArgs(l, 1, 1, "No arguments expected for vector:gety");
    Vec3d* v3 = this_vector(l);
    lua_Number y;
    y = static_cast<lua_Number>(v3->y);
    lua_pushnumber(l, y);

    return 1;
}

static int vector_getz(lua_State* l)
{
    checkArgs(l, 1, 1, "No arguments expected for vector:getz");
    Vec3d* v3 = this_vector(l);
    lua_Number z;
    z = static_cast<lua_Number>(v3->z);
    lua_pushnumber(l, z);

    return 1;
}

static int vector_normalize(lua_State* l)
{
    checkArgs(l, 1, 1, "No arguments expected for vector:normalize");
    Vec3d* v = this_vector(l);
    Vec3d vn(*v);
    vn.normalize();
    vector_new(l, vn);
    return 1;
}

static int vector_length(lua_State* l)
{
    checkArgs(l, 1, 1, "No arguments expected for vector:length");
    Vec3d* v = this_vector(l);
    double length = v->length();
    lua_pushnumber(l, (lua_Number)length);
    return 1;
}

static int vector_add(lua_State* l)
{
    checkArgs(l, 2, 2, "Need two operands for addition");
    Vec3d* v1 = NULL;
    Vec3d* v2 = NULL;
    UniversalCoord* p = NULL;

    if (istype(l, 1, _Vec3) && istype(l, 2, _Vec3))
    {
        v1 = to_vector(l, 1);
        v2 = to_vector(l, 2);
        vector_new(l, *v1 + *v2);
    }
    else
    if (istype(l, 1, _Vec3) && istype(l, 2, _Position))
    {
        v1 = to_vector(l, 1);
        p = to_position(l, 2);
        position_new(l, *p + *v1);
    }
    else
    {
        doError(l, "Bad vector addition!");
    }
    return 1;
}

static int vector_mult(lua_State* l)
{
    checkArgs(l, 2, 2, "Need two operands for multiplication");
    Vec3d* v1 = NULL;
    Vec3d* v2 = NULL;
    lua_Number s = 0.0;
    if (istype(l, 1, _Vec3) && istype(l, 2, _Vec3))
    {
        v1 = to_vector(l, 1);
        v2 = to_vector(l, 2);
        lua_pushnumber(l, *v1 * *v2);
    }
    else
    if (istype(l, 1, _Vec3) && lua_isnumber(l, 2))
    {
        v1 = to_vector(l, 1);
        s = lua_tonumber(l, 2);
        vector_new(l, *v1 * s);
    }
    else
    if (lua_isnumber(l, 1) && istype(l, 2, _Vec3))
    {
        s = lua_tonumber(l, 1);
        v1 = to_vector(l, 2);
        vector_new(l, *v1 * s);
    }
    else
    {
        doError(l, "Bad vector multiplication!");
    }
    return 1;
}

static int vector_cross(lua_State* l)
{
    checkArgs(l, 2, 2, "Need two operands for multiplication");
    Vec3d* v1 = NULL;
    Vec3d* v2 = NULL;
    if (istype(l, 1, _Vec3) && istype(l, 2, _Vec3))
    {
        v1 = to_vector(l, 1);
        v2 = to_vector(l, 2);
        vector_new(l, *v1 ^ *v2);
    }
    else
    {
        doError(l, "Bad vector multiplication!");
    }
    return 1;

}

static int vector_tostring(lua_State* l)
{
    lua_pushstring(l, "[Vector]");
    return 1;
}

static void CreateVectorMetaTable(lua_State* l)
{
    CreateClassMetatable(l, _Vec3);

    RegisterMethod(l, "__tostring", vector_tostring);
    RegisterMethod(l, "__add", vector_add);
    RegisterMethod(l, "__sub", vector_sub);
    RegisterMethod(l, "__mul", vector_mult);
    RegisterMethod(l, "__pow", vector_cross);
    RegisterMethod(l, "__index", vector_get);
    RegisterMethod(l, "__newindex", vector_set);
    RegisterMethod(l, "getx", vector_getx);
    RegisterMethod(l, "gety", vector_gety);
    RegisterMethod(l, "getz", vector_getz);
    RegisterMethod(l, "normalize", vector_normalize);
    RegisterMethod(l, "length", vector_length);

    lua_pop(l, 1); // remove metatable from stack
}

// ==================== Rotation ====================
static int rotation_new(lua_State* l, const Quatd& qd)
{
    Quatd* q = reinterpret_cast<Quatd*>(lua_newuserdata(l, sizeof(Quatd)));
    *q = qd;
    SetClass(l, _Rotation);

    return 1;
}

static Quatd* to_rotation(lua_State* l, int index)
{
    return static_cast<Quatd*>(CheckUserData(l, index, _Rotation));
}

static Quatd* this_rotation(lua_State* l)
{
    Quatd* q = to_rotation(l, 1);
    if (q == NULL)
    {
        doError(l, "Bad rotation object!");
    }

    return q;
}


static int rotation_add(lua_State* l)
{
    checkArgs(l, 2, 2, "Need two operands for add");
    Quatd* q1 = to_rotation(l, 1);
    Quatd* q2 = to_rotation(l, 2);
    if (q1 == NULL || q2 == NULL)
    {
        doError(l, "Addition only defined for two rotations");
    }
    else
    {
        Quatd result = *q1 + *q2;
        rotation_new(l, result);
    }
    return 1;
}

static int rotation_mult(lua_State* l)
{
    checkArgs(l, 2, 2, "Need two operands for multiplication");
    Quatd* r1 = NULL;
    Quatd* r2 = NULL;
    Vec3d* v = NULL;
    lua_Number s = 0.0;
    if (istype(l, 1, _Rotation) && istype(l, 2, _Rotation))
    {
        r1 = to_rotation(l, 1);
        r2 = to_rotation(l, 2);
        rotation_new(l, *r1 * *r2);
    }
    else
    if (istype(l, 1, _Rotation) && lua_isnumber(l, 2))
    {
        r1 = to_rotation(l, 1);
        s = lua_tonumber(l, 2);
        rotation_new(l, *r1 * s);
    }
    else
    if (lua_isnumber(l, 1) && istype(l, 2, _Rotation))
    {
        s = lua_tonumber(l, 1);
        r1 = to_rotation(l, 2);
        rotation_new(l, *r1 * s);
    }
    else
    if (istype(l, 1, _Rotation) && istype(l, 2, _Vec3))
    {
        r1 = to_rotation(l, 1);
        v = to_vector(l, 2);
        rotation_new(l, *v * *r1);
    }
    else
    {
        doError(l, "Bad rotation multiplication!");
    }
    return 1;
}

static int rotation_imag(lua_State* l)
{
    checkArgs(l, 1, 1, "No arguments expected for rotation_imag");
    Quatd* q = this_rotation(l);
    vector_new(l, imag(*q));
    return 1;
}

static int rotation_real(lua_State* l)
{
    checkArgs(l, 1, 1, "No arguments expected for rotation_real");
    Quatd* q = this_rotation(l);
    lua_pushnumber(l, real(*q));
    return 1;
}

static int rotation_get(lua_State* l)
{
    checkArgs(l, 2, 2, "Invalid access of rotation-component");
    Quatd* q3 = this_rotation(l);
    string key = safeGetString(l, 2, AllErrors, "Invalid key in rotation-access");
    double value = 0.0;
    if (key == "x")
        value = imag(*q3).x;
    else if (key == "y")
        value = imag(*q3).y;
    else if (key == "z")
        value = imag(*q3).z;
    else if (key == "w")
        value = real(*q3);
    else
    {
        if (lua_getmetatable(l, 1))
        {
            lua_pushvalue(l, 2);
            lua_rawget(l, -2);
            return 1;
        }
        else
        {
            doError(l, "Internal error: couldn't get metatable");
        }
    }
    lua_pushnumber(l, (lua_Number)value);
    return 1;
}

static int rotation_set(lua_State* l)
{
    checkArgs(l, 3, 3, "Invalid access of rotation-component");
    Quatd* q3 = this_rotation(l);
    string key = safeGetString(l, 2, AllErrors, "Invalid key in rotation-access");
    double value = safeGetNumber(l, 3, AllErrors, "Rotation components must be numbers");
    Vec3d v = imag(*q3);
    double w = real(*q3);
    if (key == "x")
        v.x = value;
    else if (key == "y")
        v.y = value;
    else if (key == "z")
        v.z = value;
    else if (key == "w")
        w = value;
    else
    {
        doError(l, "Invalid key in rotation-access");
    }
    *q3 = Quatd(w, v);
    return 0;
}

static int rotation_tostring(lua_State* l)
{
    lua_pushstring(l, "[Rotation]");
    return 1;
}

static void CreateRotationMetaTable(lua_State* l)
{
    CreateClassMetatable(l, _Rotation);

    RegisterMethod(l, "real", rotation_real);
    RegisterMethod(l, "imag", rotation_imag);
    RegisterMethod(l, "__tostring", rotation_tostring);
    RegisterMethod(l, "__add", rotation_add);
    RegisterMethod(l, "__mul", rotation_mult);
    RegisterMethod(l, "__index", rotation_get);
    RegisterMethod(l, "__newindex", rotation_set);

    lua_pop(l, 1); // remove metatable from stack
}

// ==================== Position ====================
// a 128-bit per component universal coordinate
static int position_new(lua_State* l, const UniversalCoord& uc)
{
    UniversalCoord* ud = reinterpret_cast<UniversalCoord*>(lua_newuserdata(l, sizeof(UniversalCoord)));
    *ud = uc;

    SetClass(l, _Position);

    return 1;
}

static UniversalCoord* to_position(lua_State* l, int index)
{
    return static_cast<UniversalCoord*>(CheckUserData(l, index, _Position));
}

static UniversalCoord* this_position(lua_State* l)
{
    UniversalCoord* uc = to_position(l, 1);
    if (uc == NULL)
    {
        doError(l, "Bad position object!");
    }

    return uc;
}


static int position_get(lua_State* l)
{
    checkArgs(l, 2, 2, "Invalid access of position-component");
    UniversalCoord* uc = this_position(l);
    string key = safeGetString(l, 2, AllErrors, "Invalid key in position-access");
    double value = 0.0;
    if (key == "x")
        value = uc->x;
    else if (key == "y")
        value = uc->y;
    else if (key == "z")
        value = uc->z;
    else
    {
        if (lua_getmetatable(l, 1))
        {
            lua_pushvalue(l, 2);
            lua_rawget(l, -2);
            return 1;
        }
        else
        {
            doError(l, "Internal error: couldn't get metatable");
        }
    }
    lua_pushnumber(l, (lua_Number)value);
    return 1;
}

static int position_set(lua_State* l)
{
    checkArgs(l, 3, 3, "Invalid access of position-component");
    UniversalCoord* uc = this_position(l);
    string key = safeGetString(l, 2, AllErrors, "Invalid key in position-access");
    double value = safeGetNumber(l, 3, AllErrors, "Position components must be numbers");
    if (key == "x")
        uc->x = value;
    else if (key == "y")
        uc->y = value;
    else if (key == "z")
        uc->z = value;
    else
    {
        doError(l, "Invalid key in position-access");
    }
    return 0;
}

static int position_getx(lua_State* l)
{
    checkArgs(l, 1, 1, "No arguments expected for position:getx()");

    UniversalCoord* uc = this_position(l);
    lua_Number x;
    x = uc->x;
    lua_pushnumber(l, x);

    return 1;
}

static int position_gety(lua_State* l)
{
    checkArgs(l, 1, 1, "No arguments expected for position:gety()");

    UniversalCoord* uc = this_position(l);
    lua_Number y;
    y = uc->y;
    lua_pushnumber(l, y);

    return 1;
}

static int position_getz(lua_State* l)
{
    checkArgs(l, 1, 1, "No arguments expected for position:getz()");

    UniversalCoord* uc = this_position(l);
    lua_Number z;
    z = uc->z;
    lua_pushnumber(l, z);

    return 1;
}

static int position_vectorto(lua_State* l)
{
    checkArgs(l, 2, 2, "One argument expected to position:vectorto");

    UniversalCoord* uc = this_position(l);
    UniversalCoord* uc2 = to_position(l, 2);

    if (uc2 == NULL)
    {
        doError(l, "Argument to position:vectorto must be a position");
    }
    Vec3d diff = *uc2 - *uc;
    vector_new(l, diff);
    return 1;
}

static int position_orientationto(lua_State* l)
{
    checkArgs(l, 3, 3, "Two arguments expected for position:orientationto");

    UniversalCoord* src = this_position(l);
    UniversalCoord* target = to_position(l, 2);

    if (target == NULL)
    {
        doError(l, "First argument to position:orientationto must be a position");
    }

    Vec3d* upd = to_vector(l, 3);
    if (upd == NULL)
    {
        doError(l, "Second argument to position:orientationto must be a vector");
    }

    Vec3d src2target = *target - *src;
    src2target.normalize();
    Vec3d v = src2target ^ *upd;
    v.normalize();
    Vec3d u = v ^ src2target;
    Quatd qd = Quatd(Mat3d(v, u, -src2target));
    rotation_new(l, qd);

    return 1;
}

static int position_tostring(lua_State* l)
{
    // TODO: print out the coordinate as it would appear in a cel:// URL
    lua_pushstring(l, "[Position]");

    return 1;
}

static int position_distanceto(lua_State* l)
{
    checkArgs(l, 2, 2, "One argument expected to position:distanceto()");

    UniversalCoord* uc = this_position(l);
    UniversalCoord* uc2 = to_position(l, 2);
    if (uc2 == NULL)
    {
        doError(l, "Position expected as argument to position:distanceto");
    }

    Vec3d v = *uc2 - *uc;
    lua_pushnumber(l, astro::microLightYearsToKilometers(v.length()));

    return 1;
}

static int position_add(lua_State* l)
{
    checkArgs(l, 2, 2, "Need two operands for addition");
    UniversalCoord* p1 = NULL;
    UniversalCoord* p2 = NULL;
    Vec3d* v2 = NULL;

    if (istype(l, 1, _Position) && istype(l, 2, _Position))
    {
        p1 = to_position(l, 1);
        p2 = to_position(l, 2);
        // this is not very intuitive, as p1-p2 is a vector
        position_new(l, *p1 + *p2);
    }
    else
    if (istype(l, 1, _Position) && istype(l, 2, _Vec3))
    {
        p1 = to_position(l, 1);
        v2 = to_vector(l, 2);
        position_new(l, *p1 + *v2);
    }
    else
    {
        doError(l, "Bad position addition!");
    }
    return 1;
}

static int position_sub(lua_State* l)
{
    checkArgs(l, 2, 2, "Need two operands for subtraction");
    UniversalCoord* p1 = NULL;
    UniversalCoord* p2 = NULL;
    Vec3d* v2 = NULL;

    if (istype(l, 1, _Position) && istype(l, 2, _Position))
    {
        p1 = to_position(l, 1);
        p2 = to_position(l, 2);
        vector_new(l, *p1 - *p2);
    }
    else
    if (istype(l, 1, _Position) && istype(l, 2, _Vec3))
    {
        p1 = to_position(l, 1);
        v2 = to_vector(l, 2);
        position_new(l, *p1 - *v2);
    }
    else
    {
        doError(l, "Bad position subtraction!");
    }
    return 1;
}

static int position_addvector(lua_State* l)
{
    checkArgs(l, 2, 2, "One argument expected to position:addvector()");
    UniversalCoord* uc = this_position(l);
    Vec3d* v3d = to_vector(l, 2);
    if (v3d == NULL)
    {
        doError(l, "Vector expected as argument to position:addvector");
    }
    else
    if (uc != NULL && v3d != NULL)
    {
        UniversalCoord ucnew = *uc + *v3d;
        position_new(l, ucnew);
    }
    return 1;
}

static void CreatePositionMetaTable(lua_State* l)
{
    CreateClassMetatable(l, _Position);

    RegisterMethod(l, "__tostring", position_tostring);
    RegisterMethod(l, "distanceto", position_distanceto);
    RegisterMethod(l, "vectorto", position_vectorto);
    RegisterMethod(l, "orientationto", position_orientationto);
    RegisterMethod(l, "addvector", position_addvector);
    RegisterMethod(l, "__add", position_add);
    RegisterMethod(l, "__sub", position_sub);
    RegisterMethod(l, "__index", position_get);
    RegisterMethod(l, "__newindex", position_set);
    RegisterMethod(l, "getx", position_getx);
    RegisterMethod(l, "gety", position_gety);
    RegisterMethod(l, "getz", position_getz);

    lua_pop(l, 1); // remove metatable from stack
}

// ==================== FrameOfReference ====================
static int frame_new(lua_State* l, const FrameOfReference& f)
{
    FrameOfReference* ud = reinterpret_cast<FrameOfReference*>(lua_newuserdata(l, sizeof(FrameOfReference)));
    *ud = f;

    SetClass(l, _Frame);

    return 1;
}

static FrameOfReference* to_frame(lua_State* l, int index)
{
    return static_cast<FrameOfReference*>(CheckUserData(l, index, _Frame));
}

static FrameOfReference* this_frame(lua_State* l)
{
    FrameOfReference* f = to_frame(l, 1);
    if (f == NULL)
    {
        doError(l, "Bad frame object!");
    }

    return f;
}


// Convert from frame coordinates to universal.
static int frame_from(lua_State* l)
{
    checkArgs(l, 2, 3, "Two or three arguments required for frame:from");

    FrameOfReference* frame = this_frame(l);
    CelestiaCore* appCore = getAppCore(l, AllErrors);

    RigidTransform rt;

    UniversalCoord* uc = NULL;
    Quatd* q = NULL;
    double jd = 0.0;

    if (istype(l, 2, _Position))
    {
        uc = to_position(l, 2);
    }
    else if (istype(l, 2, _Rotation))
    {
        q = to_rotation(l, 2);
    }
    if (uc == NULL && q == NULL)
    {
        doError(l, "Position or rotation expected as second argument to frame:from()");
    }

    jd = safeGetNumber(l, 3, WrongType, "Second arg to frame:from must be a number", appCore->getSimulation()->getTime());

    if (uc != NULL)
    {
        rt.translation = *uc;
        rt = frame->toUniversal(rt, jd);
        position_new(l, rt.translation);
    }
    else
    {
        rt.rotation = *q;
        rt = frame->toUniversal(rt, jd);
        rotation_new(l, rt.rotation);
    }

    return 1;
}

// Convert from universal to frame coordinates.
static int frame_to(lua_State* l)
{
    checkArgs(l, 2, 3, "Two or three arguments required for frame:to");

    FrameOfReference* frame = this_frame(l);
    CelestiaCore* appCore = getAppCore(l, AllErrors);

    RigidTransform rt;

    UniversalCoord* uc = NULL;
    Quatd* q = NULL;
    double jd = 0.0;

    if (istype(l, 2, _Position))
    {
        uc = to_position(l, 2);
    }
    else
    if (istype(l, 2, _Rotation))
    {
        q = to_rotation(l, 2);
    }

    if (uc == NULL && q == NULL)
    {
        doError(l, "Position or rotation expected as second argument to frame:to()");
    }

    jd = safeGetNumber(l, 3, WrongType, "Second arg to frame:to must be a number", appCore->getSimulation()->getTime());

    if (uc != NULL)
    {
        rt.translation = *uc;
        rt = frame->fromUniversal(rt, jd);
        position_new(l, rt.translation);
    }
    else
    {
        rt.rotation = *q;
        rt = frame->fromUniversal(rt, jd);
        rotation_new(l, rt.rotation);
    }

    return 1;
}

static int frame_getrefobject(lua_State* l)
{
    checkArgs(l, 1, 1, "No arguments expected for frame:getrefobject()");

    FrameOfReference* frame = this_frame(l);
    if (frame->refObject.getType() == Selection::Type_Nil)
    {
        lua_pushnil(l);
    }
    else
    {
        object_new(l, frame->refObject);
    }
    return 1;
}

static int frame_gettargetobject(lua_State* l)
{
    checkArgs(l, 1, 1, "No arguments expected for frame:gettarget()");

    FrameOfReference* frame = this_frame(l);
    if (frame->targetObject.getType() == Selection::Type_Nil)
    {
        lua_pushnil(l);
    }
    else
    {
        object_new(l, frame->targetObject);
    }
    return 1;
}

static int frame_getcoordinatesystem(lua_State* l)
{
    checkArgs(l, 1, 1, "No arguments expected for frame:getcoordinatesystem()");

    FrameOfReference* frame = this_frame(l);
    string coordsys;
    switch (frame->coordSys)
    {
    case astro::Universal: 
        coordsys = "universal"; break;
    case astro::Ecliptical: 
        coordsys = "ecliptic"; break;
    case astro::Equatorial: 
        coordsys = "equatorial"; break;
    case astro::Geographic: 
        coordsys = "planetographic"; break;
    case astro::ObserverLocal: 
        coordsys = "observer"; break;
    case astro::PhaseLock: 
        coordsys = "lock"; break;
    case astro::Chase: 
        coordsys = "chase"; break;
    default: 
        coordsys = "invalid";
    }
    lua_pushstring(l, coordsys.c_str());
    return 1;
}

static int frame_tostring(lua_State* l)
{
    // TODO: print out the actual information about the frame
    lua_pushstring(l, "[Frame]");

    return 1;
}

static void CreateFrameMetaTable(lua_State* l)
{
    CreateClassMetatable(l, _Frame);

    RegisterMethod(l, "__tostring", frame_tostring);
    RegisterMethod(l, "to", frame_to);
    RegisterMethod(l, "from", frame_from);
    RegisterMethod(l, "getcoordinatesystem", frame_getcoordinatesystem);
    RegisterMethod(l, "getrefobject", frame_getrefobject);
    RegisterMethod(l, "gettargetobject", frame_gettargetobject);

    lua_pop(l, 1); // remove metatable from stack
}

// ==================== Object ====================
// star, planet, or deep-sky object
static int object_new(lua_State* l, const Selection& sel)
{
    Selection* ud = reinterpret_cast<Selection*>(lua_newuserdata(l, sizeof(Selection)));
    *ud = sel;

    SetClass(l, _Object);

    return 1;
}

static Selection* to_object(lua_State* l, int index)
{
    return static_cast<Selection*>(CheckUserData(l, index, _Object));
}

static Selection* this_object(lua_State* l)
{
    Selection* sel = to_object(l, 1);
    if (sel == NULL)
    {
        doError(l, "Bad position object!");
    }

    return sel;
}


static int object_tostring(lua_State* l)
{
    lua_pushstring(l, "[Object]");

    return 1;
}

static int object_radius(lua_State* l)
{
    checkArgs(l, 1, 1, "No arguments expected to function object:radius");

    Selection* sel = this_object(l);
    lua_pushnumber(l, sel->radius());

    return 1;
}

static int object_type(lua_State* l)
{
    checkArgs(l, 1, 1, "No arguments expected to function object:type");

    Selection* sel = this_object(l);
    const char* tname = "unknown";
    switch (sel->getType())
    {
    case Selection::Type_Body:
        {
            int cl = sel->body()->getClassification();
            switch (cl)
            {
            case Body::Planet     : tname = "planet"; break;
            case Body::Moon       : tname = "moon"; break;
            case Body::Asteroid   : tname = "asteroid"; break;
            case Body::Comet      : tname = "comet"; break;
            case Body::Spacecraft : tname = "spacecraft"; break;
            case Body::Invisible  : tname = "invisible"; break;
            }
        }
        break;

    case Selection::Type_Star:
        tname = "star";
        break;

    case Selection::Type_DeepSky:
        // TODO: return cluster, galaxy, or nebula as appropriate
        tname = "deepsky";
        break;

    case Selection::Type_Location:
        tname = "location";
        break;

    case Selection::Type_Nil:
        tname = "null";
        break;
    }

    lua_pushstring(l, tname);

    return 1;
}

static int object_name(lua_State* l)
{
    checkArgs(l, 1, 1, "No arguments expected to function object:name");

    Selection* sel = this_object(l);
    switch (sel->getType())
    {
    case Selection::Type_Body:
        lua_pushstring(l, sel->body()->getName().c_str());
        break;
    case Selection::Type_DeepSky:
        lua_pushstring(l, sel->deepsky()->getName().c_str());
        break;
    case Selection::Type_Star:
        lua_pushstring(l, getAppCore(l, AllErrors)->getSimulation()->getUniverse()
                       ->getStarCatalog()->getStarName(*(sel->star())).c_str());
        break;
    case Selection::Type_Location:
        lua_pushstring(l, sel->location()->getName().c_str());
        break;
    default:
        lua_pushstring(l, "?");
        break;
    }

    return 1;
}

static int object_spectraltype(lua_State* l)
{
    checkArgs(l, 1, 1, "No arguments expected to function object:spectraltype");

    Selection* sel = this_object(l);
    if (sel->star() != NULL)
    {
        char buf[16];
        StellarClass sc = sel->star()->getStellarClass();
        if (sc.str(buf, sizeof(buf)))
        {
            lua_pushstring(l, buf);
        }
        else
        {
            // This should only happen if the spectral type has > 15 chars
            // (i.e. never, unless there's a bug)
            assert(0);
            doError(l, "Bad spectral type (this is a bug!)");
        }
    }
    else
    {
        lua_pushnil(l);
    }

    return 1;
}

static int object_getinfo(lua_State* l)
{
    checkArgs(l, 1, 1, "No arguments expected to function object:getinfo");

    lua_newtable(l);

    Selection* sel = this_object(l);
    if (sel->star() != NULL)
    {
        Star* star = sel->star();
        setTable(l, "type", "star");
        setTable(l, "name", getAppCore(l, AllErrors)->getSimulation()->getUniverse()
                       ->getStarCatalog()->getStarName(*(sel->star())).c_str());
        setTable(l, "catalogNumber", star->getCatalogNumber());
        setTable(l, "stellarClass", star->getStellarClass().str().c_str());
        setTable(l, "absoluteMagnitude", (lua_Number)star->getAbsoluteMagnitude());
        setTable(l, "luminosity", (lua_Number)star->getLuminosity());
        setTable(l, "radius", (lua_Number)star->getRadius());
        setTable(l, "temperature", (lua_Number)star->getTemperature());
        setTable(l, "rotationPeriod", (lua_Number)star->getRotationPeriod());
        setTable(l, "bolometricMagnitude", (lua_Number)star->getBolometricMagnitude());
    }
    else if (sel->body() != NULL)
    {
        Body* body = sel->body();
        const char* tname = "unknown";
        switch (body->getClassification())
        {
        case Body::Planet     : tname = "planet"; break;
        case Body::Moon       : tname = "moon"; break;
        case Body::Asteroid   : tname = "asteroid"; break;
        case Body::Comet      : tname = "comet"; break;
        case Body::Spacecraft : tname = "spacecraft"; break;
        case Body::Invisible  : tname = "invisible"; break;
        }
        setTable(l, "type", tname);
        setTable(l, "name", body->getName().c_str());
        setTable(l, "mass", (lua_Number)body->getMass());
        setTable(l, "oblateness", (lua_Number)body->getOblateness());
        setTable(l, "albedo", (lua_Number)body->getAlbedo());
        setTable(l, "infoURL", body->getInfoURL().c_str());
        setTable(l, "radius", (lua_Number)body->getRadius());

        double lifespanStart, lifespanEnd;
        body->getLifespan(lifespanStart, lifespanEnd);
        setTable(l, "lifespanStart", (lua_Number)lifespanStart);
        setTable(l, "lifespanEnd", (lua_Number)lifespanEnd);
        // TODO: atmosphere, surfaces ?

        PlanetarySystem* system = body->getSystem();
        if (system->getPrimaryBody() != NULL)
        {
            Selection parent(system->getPrimaryBody());
            lua_pushstring(l, "parent");
            object_new(l, parent);
            lua_settable(l, -3);
        }
        else
        {
            Selection parent(system->getStar());
            lua_pushstring(l, "parent");
            object_new(l, parent);
            lua_settable(l, -3);
        }

        lua_pushstring(l, "hasRings");
        lua_pushboolean(l, body->getRings() != NULL);
        lua_settable(l, -3);
    }
    else if (sel->deepsky() != NULL)
    {
        setTable(l, "type", "deepsky");
        DeepSkyObject* deepsky = sel->deepsky();
        setTable(l, "name", deepsky->getName().c_str());
        setTable(l, "radius", (lua_Number)deepsky->getRadius());
    }
    else if (sel->location() != NULL)
    {
        setTable(l, "type", "location");
        Location* location = sel->location();
        setTable(l, "name", location->getName().c_str());
        setTable(l, "size", (lua_Number)location->getSize());
        setTable(l, "importance", (lua_Number)location->getImportance());
        setTable(l, "infoURL", location->getInfoURL().c_str());

        uint32 featureType = location->getFeatureType();
        string featureName("Unknown");
        for (FlagMap::const_iterator it = LocationFlagMap.begin(); it != LocationFlagMap.end(); it++)
        {
            if (it->second == featureType)
            {
                featureName = it->first;
                break;
            }
        }
        setTable(l, "featureType", featureName.c_str());

        Body* parent = location->getParentBody();
        if (parent != NULL)
        {
            Selection selection(parent);
            lua_pushstring(l, "parent");
            object_new(l, selection);
            lua_settable(l, -3);
        }
    }
    else
    {
        setTable(l, "type", "null");
    }
    return 1;
}

static int object_absmag(lua_State* l)
{
    checkArgs(l, 1, 1, "No arguments expected to function object:absmag");

    Selection* sel = this_object(l);
    if (sel->star() != NULL)
        lua_pushnumber(l, sel->star()->getAbsoluteMagnitude());
    else
        lua_pushnil(l);

    return 1;
}

static int object_mark(lua_State* l)
{
    checkArgs(l, 1, 4, "No arguments expected to function object:mark");

    Selection* sel = this_object(l);
    CelestiaCore* appCore = getAppCore(l, AllErrors);

    Color markColor(0.0f, 1.0f, 0.0f);
    const char* colorString = safeGetString(l, 2, WrongType, "First argument to object:mark must be a string");
    if (colorString != NULL)
        Color::parse(colorString, markColor);

    Marker::Symbol markSymbol = Marker::Diamond;
    const char* markerString = safeGetString(l, 3, WrongType, "Second argument to object:mark must be a string");
    if (markerString != NULL)
        markSymbol = parseMarkerSymbol(markerString);

    float markSize = (float)safeGetNumber(l, 4, WrongType, "Third arg to object:mark must be a number", 10.0);
    if (markSize < 1.0f)
        markSize = 1.0f;
    else if (markSize > 100.0f)
        markSize = 100.0f;

    Simulation* sim = appCore->getSimulation();
    sim->getUniverse()->markObject(*sel, markSize,
                                   markColor, markSymbol, 1);

    return 0;
}

static int object_unmark(lua_State* l)
{
    checkArgs(l, 1, 1, "No arguments expected to function object:unmark");

    Selection* sel = this_object(l);
    CelestiaCore* appCore = getAppCore(l, AllErrors);

    Simulation* sim = appCore->getSimulation();
    sim->getUniverse()->unmarkObject(*sel, 1);

    return 0;
}

// Return the object's current position.  A time argument is optional;
// if not provided, the current master simulation time is used.
static int object_getposition(lua_State* l)
{
    checkArgs(l, 1, 2, "Expected no or one argument to object:getposition");

    Selection* sel = this_object(l);
    CelestiaCore* appCore = getAppCore(l, AllErrors);

    double t = safeGetNumber(l, 2, WrongType, "Time expected as argument to object:getposition",
                              appCore->getSimulation()->getTime());
    position_new(l, sel->getPosition(t));

    return 1;
}

static int object_getchildren(lua_State* l)
{
    checkArgs(l, 1, 1, "No arguments expected for object:getchildren()");

    Selection* sel = this_object(l);
    CelestiaCore* appCore = getAppCore(l, AllErrors);

    Simulation* sim = appCore->getSimulation();

    lua_newtable(l);
    if (sel->star() != NULL)
    {
        SolarSystemCatalog* solarSystemCatalog = sim->getUniverse()->getSolarSystemCatalog();
        SolarSystemCatalog::iterator iter = solarSystemCatalog->find(sel->star()->getCatalogNumber());
        if (iter != solarSystemCatalog->end())
        {
            SolarSystem* solarSys = iter->second;
            for (int i = 0; i < solarSys->getPlanets()->getSystemSize(); i++)
            {
                Body* body = solarSys->getPlanets()->getBody(i);
                Selection satSel(body);
                object_new(l, satSel);
                lua_rawseti(l, -2, i + 1);
            }
        }
    }
    else if (sel->body() != NULL)
    {
        const PlanetarySystem* satellites = sel->body()->getSatellites();
        if (satellites != NULL && satellites->getSystemSize() != 0)
        {
            for (int i = 0; i < satellites->getSystemSize(); i++)
            {
                Body* body = satellites->getBody(i);
                Selection satSel(body);
                object_new(l, satSel);
                lua_rawseti(l, -2, i + 1);
            }
        }
    }

    return 1;
}

static int object_preloadtexture(lua_State* l)
{
    checkArgs(l, 1, 1, "No argument expected to object:preloadtexture");
    CelestiaCore* appCore = getAppCore(l, AllErrors);

    Renderer* renderer = appCore->getRenderer();
    Selection* sel = this_object(l);
    if (sel->body() != NULL)
    {
        if (renderer != NULL)
            renderer->loadTextures(sel->body());
    }
    return 0;
}

static void CreateObjectMetaTable(lua_State* l)
{
    CreateClassMetatable(l, _Object);

    RegisterMethod(l, "__tostring", object_tostring);
    RegisterMethod(l, "radius", object_radius);
    RegisterMethod(l, "type", object_type);
    RegisterMethod(l, "spectraltype", object_spectraltype);
    RegisterMethod(l, "getinfo", object_getinfo);
    RegisterMethod(l, "absmag", object_absmag);
    RegisterMethod(l, "name", object_name);
    RegisterMethod(l, "mark", object_mark);
    RegisterMethod(l, "unmark", object_unmark);
    RegisterMethod(l, "getposition", object_getposition);
    RegisterMethod(l, "getchildren", object_getchildren);
    RegisterMethod(l, "preloadtexture", object_preloadtexture);

    lua_pop(l, 1); // pop metatable off the stack
}

// ==================== Observer ====================
static int observer_new(lua_State* l, Observer* o)
{
    Observer** ud = static_cast<Observer**>(lua_newuserdata(l, sizeof(Observer*)));
    *ud = o;

    SetClass(l, _Observer);

    return 1;
}

static Observer* to_observer(lua_State* l, int index)
{
    Observer** o = static_cast<Observer**>(lua_touserdata(l, index));
    CelestiaCore* appCore = getAppCore(l, AllErrors);

    // Check if pointer is still valid, i.e. is used by a view:
    if (o != NULL && getViewByObserver(appCore, *o) != NULL)
    {
            return *o;
    }
    return NULL;
}

static Observer* this_observer(lua_State* l)
{
    Observer* obs = to_observer(l, 1);
    if (obs == NULL)
    {
        doError(l, "Bad observer object (maybe tried to access a deleted view?)!");
    }

    return obs;
}


static int observer_isvalid(lua_State* l)
{
    checkArgs(l, 1, 1, "No arguments expected for observer:isvalid()");
    lua_pushboolean(l, to_observer(l, 1) != NULL);
    return 1;
}

static int observer_tostring(lua_State* l)
{
    lua_pushstring(l, "[Observer]");

    return 1;
}

static int observer_setposition(lua_State* l)
{
    checkArgs(l, 2, 2, "One argument required for setpos");

    Observer* o = this_observer(l);

    UniversalCoord* uc = to_position(l,2);
    if (uc == NULL)
    {
        doError(l, "Argument to observer:setposition must be a position");
    }
    o->setPosition(*uc);
    return 0;
}

static int observer_setorientation(lua_State* l)
{
    checkArgs(l, 2, 2, "One argument required for setorientation");

    Observer* o = this_observer(l);

    Quatd* q = to_rotation(l,2);
    if (q == NULL)
    {
        doError(l, "Argument to observer:setorientation must be a rotation");
    }
    o->setOrientation(*q);
    return 0;
}

static int observer_getorientation(lua_State* l)
{
    checkArgs(l, 1, 1, "No arguments expected to observer:getorientation()");

    Observer* o = this_observer(l);
    Quatf q = o->getOrientation();
    rotation_new(l, Quatd(q.w, q.x, q.y, q.z));

    return 1;
}

static int observer_rotate(lua_State* l)
{
    checkArgs(l, 2, 2, "One argument required for rotate");

    Observer* o = this_observer(l);

    Quatd* q = to_rotation(l,2);
    if (q == NULL)
    {
        doError(l, "Argument to observer:setpos must be a rotation");
    }
    Quatf qf((float) q->w, (float) q->x, (float) q->y, (float) q->z);
    o->rotate(qf);
    return 0;
}

static int observer_lookat(lua_State* l)
{
    checkArgs(l, 3, 4, "Two or three arguments required for lookat");
    int argc = lua_gettop(l);

    Observer* o = this_observer(l);

    UniversalCoord* from = NULL;
    UniversalCoord* to = NULL;
    Vec3d* upd = NULL;
    if (argc == 3)
    {
        to = to_position(l, 2);
        upd = to_vector(l, 3);
        if (to == NULL)
        {
            doError(l, "Argument 1 (of 2) to observer:lookat must be of type position");
        }
    }
    else
    if (argc == 4)
    {
        from = to_position(l, 2);
        to = to_position(l, 3);
        upd = to_vector(l, 4);

        if (to == NULL || from == NULL)
        {
            doError(l, "Argument 1 and 2 (of 3) to observer:lookat must be of type position");
        }
    }
    if (upd == NULL)
    {
        doError(l, "Last argument to observer:lookat must be of type vector");
    }
    Vec3d nd;
    if (from == NULL)
    {
        nd = (*to) - o->getPosition();
    }
    else
    {
        nd = (*to) - (*from);
    }
    // need Vec3f instead:
    Vec3f up = Vec3f((float) upd->x, (float) upd->y, (float) upd->z);
    Vec3f n = Vec3f((float) nd.x, (float) nd.y, (float) nd.z);

    n.normalize();
    Vec3f v = n ^ up;
    v.normalize();
    Vec3f u = v ^ n;
    Quatf qf = Quatf(Mat3f(v, u, -n));
    o->setOrientation(qf);
    return 0;
}

static int observer_gototable(lua_State* l)
{
    checkArgs(l, 2, 2, "Expected one table as argument to goto");

    Observer* o = this_observer(l);
    if (!lua_istable(l, 2))
    {
        lua_pushstring(l, "Argument to goto must be a table");
    }

    Observer::JourneyParams jparams;
    jparams.duration = 5.0;
    jparams.from = o->getSituation().translation;
    jparams.to = o->getSituation().translation;
    jparams.initialOrientation = o->getOrientation();
    jparams.finalOrientation = o->getOrientation();
    jparams.startInterpolation = 0.25;
    jparams.endInterpolation = 0.75;
    jparams.accelTime = 0.5;
    jparams.traj = Observer::Linear;

    lua_pushstring(l, "duration");
    lua_gettable(l, 2);
    jparams.duration = safeGetNumber(l, 3, NoErrors, "", 5.0);
    lua_settop(l, 2);

    lua_pushstring(l, "from");
    lua_gettable(l, 2);
    UniversalCoord* from = to_position(l, 3);
    if (from != NULL)
        jparams.from = *from;
    lua_settop(l, 2);

    lua_pushstring(l, "to");
    lua_gettable(l, 2);
    UniversalCoord* to = to_position(l, 3);
    if (to != NULL)
        jparams.to = *to;
    lua_settop(l, 2);

    lua_pushstring(l, "initialOrientation");
    lua_gettable(l, 2);
    Quatd* rot1 = to_rotation(l, 3);
    if (rot1 != NULL)
        jparams.initialOrientation = Quatf((float) rot1->w, (float) rot1->x, (float) rot1->y, (float) rot1->z);
    lua_settop(l, 2);

    lua_pushstring(l, "finalOrientation");
    lua_gettable(l, 2);
    Quatd* rot2 = to_rotation(l, 3);
    if (rot2 != NULL)
        jparams.finalOrientation = Quatf((float) rot2->w, (float) rot2->x, (float) rot2->y, (float) rot2->z);
    lua_settop(l, 2);

    lua_pushstring(l, "startInterpolation");
    lua_gettable(l, 2);
    jparams.startInterpolation = safeGetNumber(l, 3, NoErrors, "", 0.25);
    lua_settop(l, 2);

    lua_pushstring(l, "endInterpolation");
    lua_gettable(l, 2);
    jparams.endInterpolation = safeGetNumber(l, 3, NoErrors, "", 0.75);
    lua_settop(l, 2);

    lua_pushstring(l, "accelTime");
    lua_gettable(l, 2);
    jparams.accelTime = safeGetNumber(l, 3, NoErrors, "", 0.5);
    lua_settop(l, 2);

    jparams.duration = max(0.0, jparams.duration);
    jparams.accelTime = min(1.0, max(0.1, jparams.accelTime));
    jparams.startInterpolation = min(1.0, max(0.0, jparams.startInterpolation));
    jparams.endInterpolation = min(1.0, max(0.0, jparams.endInterpolation));

    // args are in universal coords, let setFrame handle conversion:
    FrameOfReference tmp = o->getFrame();
    o->setFrame(FrameOfReference());
    o->gotoJourney(jparams);
    o->setFrame(tmp);
    return 0;
}

// First argument is the target object or position; optional second argument
// is the travel time
static int observer_goto(lua_State* l)
{
    if (lua_gettop(l) == 2 && lua_istable(l, 2))
    {
        // handle this in own function
        return observer_gototable(l);
    }
    checkArgs(l, 1, 5, "One to four arguments expected to observer:goto");

    Observer* o = this_observer(l);

    Selection* sel = to_object(l, 2);
    UniversalCoord* uc = to_position(l, 2);
    if (sel == NULL && uc == NULL)
    {
        doError(l, "First arg to observer:goto must be object or position");
    }

    double travelTime = safeGetNumber(l, 3, WrongType, "Second arg to observer:goto must be a number", 5.0);
    double startInter = safeGetNumber(l, 4, WrongType, "Third arg to observer:goto must be a number", 0.25);
    double endInter = safeGetNumber(l, 5, WrongType, "Fourth arg to observer:goto must be a number", 0.75);
    if (startInter < 0 || startInter > 1) startInter = 0.25;
    if (endInter < 0 || endInter > 1) startInter = 0.75;

    // The first argument may be either an object or a position
    if (sel != NULL)
    {
        o->gotoSelection(*sel, travelTime, startInter, endInter, Vec3f(0, 1, 0), astro::ObserverLocal);
    }
    else
    {
        RigidTransform rt = o->getSituation();
        rt.translation = *uc;
        o->gotoLocation(rt, travelTime);
    }

    return 0;
}

static int observer_gotolonglat(lua_State* l)
{
    checkArgs(l, 2, 7, "One to five arguments expected to observer:gotolonglat");

    Observer* o = this_observer(l);

    Selection* sel = to_object(l, 2);
    if (sel == NULL)
    {
        doError(l, "First arg to observer:gotolonglat must be an object");
    }
    double defaultDistance = sel->radius() * 5.0;

    double longitude  = safeGetNumber(l, 3, WrongType, "Second arg to observer:gotolonglat must be a number", 0.0);
    double latitude   = safeGetNumber(l, 4, WrongType, "Third arg to observer:gotolonglat must be a number", 0.0);
    double distance   = safeGetNumber(l, 5, WrongType, "Fourth arg to observer:gotolonglat must be a number", defaultDistance);
    double travelTime = safeGetNumber(l, 6, WrongType, "Fifth arg to observer:gotolonglat must be a number", 5.0);

    distance = distance / KM_PER_LY;

    Vec3f up(0.0f, 1.0f, 0.0f);
    if (lua_gettop(l) >= 7)
    {
        Vec3d* uparg = to_vector(l, 7);
        if (uparg == NULL)
        {
            doError(l, "Sixth argument to observer:gotolonglat must be a vector");
        }
        up = Vec3f((float)uparg->x, (float)uparg->y, (float)uparg->z);
    }
    o->gotoSelectionLongLat(*sel, travelTime, distance, (float)longitude, (float)latitude, up);

    return 0;
}

// deprecated: wrong name, bad interface.
static int observer_gotolocation(lua_State* l)
{
    checkArgs(l, 2, 3,"Expected one or two arguments to observer:gotolocation");

    Observer* o = this_observer(l);

    double travelTime = safeGetNumber(l, 3, WrongType, "Second arg to observer:gotolocation must be a number", 5.0);
    if (travelTime < 0)
        travelTime = 0.0;

    UniversalCoord* uc = to_position(l, 2);
    if (uc != NULL)
    {
        RigidTransform rt = o->getSituation();
        rt.translation = *uc;
        o->gotoLocation(rt, travelTime);
    }
    else
    {
        doError(l, "First arg to observer:gotolocation must be a position");
    }

    return 0;
}

static int observer_gotodistance(lua_State* l)
{
    checkArgs(l, 2, 5, "One to four arguments expected to observer:gotodistance");

    Observer* o = this_observer(l);
    Selection* sel = to_object(l, 2);
    if (sel == NULL)
    {
        doError(l, "First arg to observer:gotodistance must be object");
    }

    double distance = safeGetNumber(l, 3, WrongType, "Second arg to observer:gotodistance must be a number", 20000);
    double travelTime = safeGetNumber(l, 4, WrongType, "Third arg to observer:gotodistance must be a number", 5.0);

    Vec3f up(0,1,0);
    if (lua_gettop(l) > 4)
    {
        Vec3d* up_arg = to_vector(l, 5);
        if (up_arg == NULL)
        {
            doError(l, "Fourth arg to observer:gotodistance must be a vector");
        }
        up.x = (float)up_arg->x;
        up.y = (float)up_arg->y;
        up.z = (float)up_arg->z;
    }

    o->gotoSelection(*sel, travelTime, astro::kilometersToLightYears(distance), up, astro::Universal);

    return 0;
}

static int observer_gotosurface(lua_State* l)
{
    checkArgs(l, 2, 3, "One to two arguments expected to observer:gotosurface");

    Observer* o = this_observer(l);
    Selection* sel = to_object(l, 2);
    if (sel == NULL)
    {
        doError(l, "First arg to observer:gotosurface must be object");
    }

    double travelTime = safeGetNumber(l, 3, WrongType, "Second arg to observer:gotosurface must be a number", 5.0);

    // This is needed because gotoSurface expects frame to be geosync:
    o->geosynchronousFollow(*sel);
    o->gotoSurface(*sel, travelTime);

    return 0;
}

static int observer_center(lua_State* l)
{
    checkArgs(l, 2, 3, "Expected one or two arguments for to observer:center");

    Observer* o = this_observer(l);
    Selection* sel = to_object(l, 2);
    if (sel == NULL)
    {
        doError(l, "First argument to observer:center must be an object");
    }
    double travelTime = safeGetNumber(l, 3, WrongType, "Second arg to observer:center must be a number", 5.0);

    o->centerSelection(*sel, travelTime);

    return 0;
}

static int observer_centerorbit(lua_State* l)
{
    checkArgs(l, 2, 3, "Expected one or two arguments for to observer:center");

    Observer* o = this_observer(l);
    Selection* sel = to_object(l, 2);
    if (sel == NULL)
    {
        doError(l, "First argument to observer:centerorbit must be an object");
    }
    double travelTime = safeGetNumber(l, 3, WrongType, "Second arg to observer:centerorbit must be a number", 5.0);

    o->centerSelectionCO(*sel, travelTime);

    return 0;
}

static int observer_cancelgoto(lua_State* l)
{
    checkArgs(l, 1, 1, "Expected no arguments to observer:cancelgoto");

    Observer* o = this_observer(l);
    o->cancelMotion();

    return 0;
}

static int observer_follow(lua_State* l)
{
    checkArgs(l, 2, 2, "One argument expected for observer:follow");

    Observer* o = this_observer(l);
    Selection* sel = to_object(l, 2);
    if (sel == NULL)
    {
        doError(l, "First argument to observer:follow must be an object");
    }
    o->follow(*sel);

    return 0;
}

static int observer_synchronous(lua_State* l)
{
    checkArgs(l, 2, 2, "One argument expected for observer:synchronous");

    Observer* o = this_observer(l);
    Selection* sel = to_object(l, 2);
    if (sel == NULL)
    {
        doError(l, "First argument to observer:synchronous must be an object");
    }
    o->geosynchronousFollow(*sel);

    return 0;
}

static int observer_lock(lua_State* l)
{
    checkArgs(l, 2, 2, "One argument expected for observer:lock");

    Observer* o = this_observer(l);
    Selection* sel = to_object(l, 2);
    if (sel == NULL)
    {
        doError(l, "First argument to observer:phaseLock must be an object");
    }
    o->phaseLock(*sel);

    return 0;
}

static int observer_chase(lua_State* l)
{
    checkArgs(l, 2, 2, "One argument expected for observer:chase");

    Observer* o = this_observer(l);
    Selection* sel = to_object(l, 2);
    if (sel == NULL)
    {
        doError(l, "First argument to observer:chase must be an object");
    }
    o->chase(*sel);

    return 0;
}

static int observer_track(lua_State* l)
{
    checkArgs(l, 2, 2, "One argument expected for observer:track");

    Observer* o = this_observer(l);

    // If the argument is nil, clear the tracked object
    if (lua_isnil(l, 2))
    {
        o->setTrackedObject(Selection());
    }
    else
    {
        // Otherwise, turn on tracking and set the tracked object
        Selection* sel = to_object(l, 2);
        if (sel == NULL)
        {
            doError(l, "First argument to observer:center must be an object");
        }
        o->setTrackedObject(*sel);
    }

    return 0;
}

// Return true if the observer is still moving as a result of a goto, center,
// or similar command.
static int observer_travelling(lua_State* l)
{
    checkArgs(l, 1, 1, "No arguments expected to observer:travelling");

    Observer* o = this_observer(l);
    if (o->getMode() == Observer::Travelling)
        lua_pushboolean(l, 1);
    else
        lua_pushboolean(l, 0);

    return 1;
}

// Return the observer's current time as a Julian day number
static int observer_gettime(lua_State* l)
{
    checkArgs(l, 1, 1, "No arguments expected to observer:gettime");

    Observer* o = this_observer(l);
    lua_pushnumber(l, o->getTime());

    return 1;
}

// Return the observer's current position
static int observer_getposition(lua_State* l)
{
    checkArgs(l, 1, 1, "No arguments expected to observer:getposition");

    Observer* o = this_observer(l);
    position_new(l, o->getPosition());

    return 1;
}

static int observer_getsurface(lua_State* l)
{
    checkArgs(l, 1, 1, "One argument expected to observer:getsurface()");

    Observer* obs = this_observer(l);
    lua_pushstring(l, obs->getDisplayedSurface().c_str());

    return 1;
}

static int observer_setsurface(lua_State* l)
{
    checkArgs(l, 2, 2, "One argument expected to observer:setsurface()");

    Observer* obs = this_observer(l);
    const char* s = lua_tostring(l, 2);

    if (s == NULL)
        obs->setDisplayedSurface("");
    else
        obs->setDisplayedSurface(s);

    return 0;
}

static int observer_getframe(lua_State* l)
{
    checkArgs(l, 1, 1, "No arguments expected for observer:getframe()");

    Observer* obs = this_observer(l);

    FrameOfReference frame = obs->getFrame();
    frame_new(l, frame);
    return 1;
}

static int observer_setframe(lua_State* l)
{
    checkArgs(l, 2, 2, "One argument required for observer:setframe()");

    Observer* obs = this_observer(l);

    FrameOfReference* frame;
    frame = to_frame(l, 2);
    if (frame != NULL)
    {
        obs->setFrame(*frame);
    }
    else
    {
        doError(l, "Argument to observer:setframe must be a frame");
    }
    return 0;
}

static int observer_setspeed(lua_State* l)
{
    checkArgs(l, 2, 2, "One argument required for observer:setspeed()");

    Observer* obs = this_observer(l);

    double speed = safeGetNumber(l, 2, AllErrors, "First argument to observer:setspeed must be a number");
    obs->setTargetSpeed((float)speed);
    return 0;
}

static int observer_getspeed(lua_State* l)
{
    checkArgs(l, 1, 1, "No argument expected for observer:getspeed()");

    Observer* obs = this_observer(l);

    lua_pushnumber(l, (lua_Number)obs->getTargetSpeed());
    return 1;
}

static int observer_setfov(lua_State* l)
{
    checkArgs(l, 2, 2, "One argument expected to observer:setfov()");

    Observer* obs = this_observer(l);
    double fov = safeGetNumber(l, 2, AllErrors, "Argument to observer:setfov() must be a number");
    if ((fov >= degToRad(0.001f)) && (fov <= degToRad(120.0f)))
    {
        obs->setFOV((float) fov);
        getAppCore(l, AllErrors)->setZoomFromFOV();
    }
    return 0;
}

static int observer_getfov(lua_State* l)
{
    checkArgs(l, 1, 1, "No argument expected to observer:getfov()");

    Observer* obs = this_observer(l);
    lua_pushnumber(l, obs->getFOV());
    return 1;
}

static int observer_splitview(lua_State* l)
{
    checkArgs(l, 2, 3, "One or two arguments expected for observer:splitview()");

    Observer* obs = this_observer(l);
    CelestiaCore* appCore = getAppCore(l, AllErrors);
    const char* splitType = safeGetString(l, 2, AllErrors, "First argument to observer:splitview() must be a string");
    View::Type type = (compareIgnoringCase(splitType, "h") == 0) ? View::HorizontalSplit : View::VerticalSplit;
    double splitPos = safeGetNumber(l, 3, WrongType, "Number expected as argument to observer:splitview()", 0.5);
    if (splitPos < 0.1)
        splitPos = 0.1;
    if (splitPos > 0.9)
        splitPos = 0.9;
    View* view = getViewByObserver(appCore, obs);
    appCore->splitView(type, view, (float)splitPos);
    return 0;
}

static int observer_deleteview(lua_State* l)
{
    checkArgs(l, 1, 1, "No argument expected for observer:deleteview()");

    Observer* obs = this_observer(l);
    CelestiaCore* appCore = getAppCore(l, AllErrors);
    View* view = getViewByObserver(appCore, obs);
    appCore->deleteView(view);
    return 0;
}

static int observer_singleview(lua_State* l)
{
    checkArgs(l, 1, 1, "No argument expected for observer:singleview()");

    Observer* obs = this_observer(l);
    CelestiaCore* appCore = getAppCore(l, AllErrors);
    View* view = getViewByObserver(appCore, obs);
    appCore->singleView(view);
    return 0;
}

static int observer_equal(lua_State* l)
{
    checkArgs(l, 2, 2, "Wrong number of arguments for comparison!");

    Observer* o1 = this_observer(l);
    Observer* o2 = to_observer(l, 2);

    lua_pushboolean(l, (o1 == o2));
    return 1;
}

static int observer_setlocationflags(lua_State* l)
{
    checkArgs(l, 2, 2, "One argument expected for observer:setlocationflags()");
    Observer* obs = this_observer(l);
    if (!lua_istable(l, 2))
    {
        doError(l, "Argument to observer:setlocationflags() must be a table");
    }

    lua_pushnil(l);
    int locationFlags = obs->getLocationFilter();
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
            doError(l, "Keys in table-argument to observer:setlocationflags() must be strings");
        }
        if (lua_isboolean(l, -1))
        {
            value = lua_toboolean(l, -1);
        }
        else
        {
            doError(l, "Values in table-argument to observer:setlocationflags() must be boolean");
        }
        if (LocationFlagMap.count(key) == 0)
        {
            cerr << "Unknown key: " << key << "\n";
        }
        else
        {
            int flag = LocationFlagMap[key];
            if (value)
            {
                locationFlags |= flag;
            }
            else
            {
                locationFlags &= ~flag;
            }
        }
        lua_pop(l,1);
    }
    obs->setLocationFilter(locationFlags);
    return 0;
}

static int observer_getlocationflags(lua_State* l)
{
    checkArgs(l, 1, 1, "No arguments expected for observer:getlocationflags()");
    Observer* obs = this_observer(l);
    lua_newtable(l);
    FlagMap::const_iterator it = LocationFlagMap.begin();
    const int locationFlags = obs->getLocationFilter();
    while (it != LocationFlagMap.end())
    {
        string key = it->first;
        lua_pushstring(l, key.c_str());
        lua_pushboolean(l, (it->second & locationFlags) != 0);
        lua_settable(l,-3);
        it++;
    }
    return 1;
}

static void CreateObserverMetaTable(lua_State* l)
{
    CreateClassMetatable(l, _Observer);

    RegisterMethod(l, "__tostring", observer_tostring);
    RegisterMethod(l, "isvalid", observer_isvalid);
    RegisterMethod(l, "goto", observer_goto);
    RegisterMethod(l, "gotolonglat", observer_gotolonglat);
    RegisterMethod(l, "gotolocation", observer_gotolocation);
    RegisterMethod(l, "gotodistance", observer_gotodistance);
    RegisterMethod(l, "gotosurface", observer_gotosurface);
    RegisterMethod(l, "cancelgoto", observer_cancelgoto);
    RegisterMethod(l, "setposition", observer_setposition);
    RegisterMethod(l, "lookat", observer_lookat);
    RegisterMethod(l, "setorientation", observer_setorientation);
    RegisterMethod(l, "getorientation", observer_getorientation);
    RegisterMethod(l, "getspeed", observer_getspeed);
    RegisterMethod(l, "setspeed", observer_setspeed);
    RegisterMethod(l, "getfov", observer_getfov);
    RegisterMethod(l, "setfov", observer_setfov);
    RegisterMethod(l, "rotate", observer_rotate);
    RegisterMethod(l, "center", observer_center);
    RegisterMethod(l, "centerorbit", observer_centerorbit);
    RegisterMethod(l, "follow", observer_follow);
    RegisterMethod(l, "synchronous", observer_synchronous);
    RegisterMethod(l, "chase", observer_chase);
    RegisterMethod(l, "lock", observer_lock);
    RegisterMethod(l, "track", observer_track);
    RegisterMethod(l, "travelling", observer_travelling);
    RegisterMethod(l, "getframe", observer_getframe);
    RegisterMethod(l, "setframe", observer_setframe);
    RegisterMethod(l, "gettime", observer_gettime);
    RegisterMethod(l, "getposition", observer_getposition);
    RegisterMethod(l, "getsurface", observer_getsurface);
    RegisterMethod(l, "setsurface", observer_setsurface);
    RegisterMethod(l, "splitview", observer_splitview);
    RegisterMethod(l, "deleteview", observer_deleteview);
    RegisterMethod(l, "singleview", observer_singleview);
    RegisterMethod(l, "getlocationflags", observer_getlocationflags);
    RegisterMethod(l, "setlocationflags", observer_setlocationflags);
    RegisterMethod(l, "__eq", observer_equal);

    lua_pop(l, 1); // remove metatable from stack
}


// ==================== Celscript-object ====================

// create a CelScriptWrapper from a string:
static int celscript_from_string(lua_State* l, string& script_text)
{
#ifdef HAVE_SSTREAM
    istringstream scriptfile(script_text);
#else
    istrstream scriptfile(script_text.c_str());
#endif
    CelestiaCore* appCore = getAppCore(l, AllErrors);
    CelScriptWrapper* celscript = new CelScriptWrapper(*appCore, scriptfile);
    if (celscript->getErrorMessage() != "")
    {
        string error = celscript->getErrorMessage();
        delete celscript;
        doError(l, error.c_str());
    }
    else
    {
        CelScriptWrapper** ud = reinterpret_cast<CelScriptWrapper**>(lua_newuserdata(l, sizeof(CelScriptWrapper*)));
        *ud = celscript;
        SetClass(l, _CelScript);
    }

    return 1;
}

static CelScriptWrapper* this_celscript(lua_State* l)
{
    CelScriptWrapper** script = static_cast<CelScriptWrapper**>(CheckUserData(l, 1, _CelScript));
    if (script == NULL)
    {
        doError(l, "Bad CEL-script object!");
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
    CreateClassMetatable(l, _CelScript);

    RegisterMethod(l, "__tostring", celscript_tostring);
    RegisterMethod(l, "tick", celscript_tick);
    RegisterMethod(l, "__gc", celscript_gc);

    lua_pop(l, 1); // remove metatable from stack
}


// ==================== Celestia-object ====================
static int celestia_new(lua_State* l, CelestiaCore* appCore)
{
    CelestiaCore** ud = reinterpret_cast<CelestiaCore**>(lua_newuserdata(l, sizeof(CelestiaCore*)));
    *ud = appCore;

    SetClass(l, _Celestia);

    return 1;
}

static CelestiaCore* to_celestia(lua_State* l, int index)
{
    CelestiaCore** appCore = static_cast<CelestiaCore**>(CheckUserData(l, index, _Celestia));
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
        doError(l, "Bad celestia object!");
    }

    return appCore;
}


static int celestia_flash(lua_State* l)
{
    checkArgs(l, 2, 3, "One or two arguments expected to function celestia:flash");

    CelestiaCore* appCore = this_celestia(l);
    const char* s = safeGetString(l, 2, AllErrors, "First argument to celestia:flash must be a string");
    double duration = safeGetNumber(l, 3, WrongType, "Second argument to celestia:flash must be a number", 1.5);
    if (duration < 0.0)
    {
        duration = 1.5;
    }

    appCore->flash(s, duration);

    return 0;
}

static int celestia_print(lua_State* l)
{
    checkArgs(l, 2, 7, "One to six arguments expected to function celestia:print");

    CelestiaCore* appCore = this_celestia(l);
    const char* s = safeGetString(l, 2, AllErrors, "First argument to celestia:print must be a string");
    double duration = safeGetNumber(l, 3, WrongType, "Second argument to celestia:print must be a number", 1.5);
    int horig = (int)safeGetNumber(l, 4, WrongType, "Third argument to celestia:print must be a number", -1.0);
    int vorig = (int)safeGetNumber(l, 5, WrongType, "Fourth argument to celestia:print must be a number", -1.0);
    int hoff = (int)safeGetNumber(l, 6, WrongType, "Fifth argument to celestia:print must be a number", 0.0);
    int voff = (int)safeGetNumber(l, 7, WrongType, "Sixth argument to celestia:print must be a number", 5.0);

    if (duration < 0.0)
    {
        duration = 1.5;
    }

    appCore->showText(s, horig, vorig, hoff, voff, duration);

    return 0;
}

static int celestia_show(lua_State* l)
{
    checkArgs(l, 1, 1000, "Wrong number of arguments to celestia:show");
    CelestiaCore* appCore = this_celestia(l);

    int argc = lua_gettop(l);
    int flags = 0;
    for (int i = 2; i <= argc; i++)
    {
        string renderFlag = safeGetString(l, i, AllErrors, "Arguments to celestia:show() must be strings");
        if (renderFlag == "lightdelay")
            appCore->setLightDelayActive(true);
        else
        if (RenderFlagMap.count(renderFlag) > 0)
            flags |= RenderFlagMap[renderFlag];
    }

    Renderer* r = appCore->getRenderer();
    r->setRenderFlags(r->getRenderFlags() | flags);

    return 0;
}

static int celestia_hide(lua_State* l)
{
    checkArgs(l, 1, 1000, "Wrong number of arguments to celestia:hide");
    CelestiaCore* appCore = this_celestia(l);

    int argc = lua_gettop(l);
    int flags = 0;
    for (int i = 2; i <= argc; i++)
    {
        string renderFlag = safeGetString(l, i, AllErrors, "Arguments to celestia:hide() must be strings"); 
        if (renderFlag == "lightdelay")
            appCore->setLightDelayActive(false);
        else
        if (RenderFlagMap.count(renderFlag) > 0)
            flags |= RenderFlagMap[renderFlag];
    }

    Renderer* r = appCore->getRenderer();
    r->setRenderFlags(r->getRenderFlags() & ~flags);

    return 0;
}

static int celestia_setrenderflags(lua_State* l)
{
    checkArgs(l, 2, 2, "One argument expected for celestia:setrenderflags()");
    CelestiaCore* appCore = this_celestia(l);
    if (!lua_istable(l, 2))
    {
        doError(l, "Argument to celestia:setrenderflags() must be a table");
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
            doError(l, "Keys in table-argument to celestia:setrenderflags() must be strings");
        }
        if (lua_isboolean(l, -1))
        {
            value = lua_toboolean(l, -1);
        }
        else
        {
            doError(l, "Values in table-argument to celestia:setrenderflags() must be boolean");
        }
        if (key == "lightdelay")
        {
            appCore->setLightDelayActive(value);
        }
        else if (RenderFlagMap.count(key) > 0)
        {
            int flag = RenderFlagMap[key];
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
    return 0;
}

static int celestia_getrenderflags(lua_State* l)
{
    checkArgs(l, 1, 1, "No arguments expected for celestia:getrenderflags()");
    CelestiaCore* appCore = this_celestia(l);
    lua_newtable(l);
    FlagMap::const_iterator it = RenderFlagMap.begin();
    const int renderFlags = appCore->getRenderer()->getRenderFlags();
    while (it != RenderFlagMap.end())
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

static int celestia_showlabel(lua_State* l)
{
    checkArgs(l, 1, 1000, "Bad method call!");
    CelestiaCore* appCore = this_celestia(l);

    int argc = lua_gettop(l);
    int flags = 0;
    for (int i = 2; i <= argc; i++)
    {
        string labelFlag = safeGetString(l, i, AllErrors, "Arguments to celestia:showlabel() must be strings"); 
        if (LabelFlagMap.count(labelFlag) > 0)
            flags |= LabelFlagMap[labelFlag];
    }

    Renderer* r = appCore->getRenderer();
    r->setLabelMode(r->getLabelMode() | flags);

    return 0;
}

static int celestia_hidelabel(lua_State* l)
{
    checkArgs(l, 1, 1000, "Invalid number of arguments in celestia:hidelabel");
    CelestiaCore* appCore = this_celestia(l);

    int argc = lua_gettop(l);
    int flags = 0;
    for (int i = 2; i <= argc; i++)
    {
        string labelFlag = safeGetString(l, i, AllErrors, "Arguments to celestia:hidelabel() must be strings"); 
        if (LabelFlagMap.count(labelFlag) > 0)
            flags |= LabelFlagMap[labelFlag];
    }

    Renderer* r = appCore->getRenderer();
    r->setLabelMode(r->getLabelMode() & ~flags);

    return 0;
}

static int celestia_setlabelflags(lua_State* l)
{
    checkArgs(l, 2, 2, "One argument expected for celestia:setlabelflags()");
    CelestiaCore* appCore = this_celestia(l);
    if (!lua_istable(l, 2))
    {
        doError(l, "Argument to celestia:setlabelflags() must be a table");
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
            doError(l, "Keys in table-argument to celestia:setlabelflags() must be strings");
        }
        if (lua_isboolean(l, -1))
        {
            value = lua_toboolean(l, -1);
        }
        else
        {
            doError(l, "Values in table-argument to celestia:setlabelflags() must be boolean");
        }
        if (LabelFlagMap.count(key) == 0)
        {
            cerr << "Unknown key: " << key << "\n";
        }
        else
        {
            int flag = LabelFlagMap[key];
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
    return 0;
}

static int celestia_getlabelflags(lua_State* l)
{
    checkArgs(l, 1, 1, "No arguments expected for celestia:getlabelflags()");
    CelestiaCore* appCore = this_celestia(l);
    lua_newtable(l);
    FlagMap::const_iterator it = LabelFlagMap.begin();
    const int labelFlags = appCore->getRenderer()->getLabelMode();
    while (it != LabelFlagMap.end())
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
    checkArgs(l, 2, 2, "One argument expected for celestia:setorbitflags()");
    CelestiaCore* appCore = this_celestia(l);
    if (!lua_istable(l, 2))
    {
        doError(l, "Argument to celestia:setorbitflags() must be a table");
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
            doError(l, "Keys in table-argument to celestia:setorbitflags() must be strings");
        }
        if (lua_isboolean(l, -1))
        {
            value = lua_toboolean(l, -1);
        }
        else
        {
            doError(l, "Values in table-argument to celestia:setorbitflags() must be boolean");
        }
        if (BodyTypeMap.count(key) == 0)
        {
            cerr << "Unknown key: " << key << "\n";
        }
        else
        {
            int flag = BodyTypeMap[key];
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
    checkArgs(l, 1, 1, "No arguments expected for celestia:getorbitflags()");
    CelestiaCore* appCore = this_celestia(l);
    lua_newtable(l);
    FlagMap::const_iterator it = BodyTypeMap.begin();
    const int orbitFlags = appCore->getRenderer()->getOrbitMask();
    while (it != BodyTypeMap.end())
    {
        string key = it->first;
        lua_pushstring(l, key.c_str());
        lua_pushboolean(l, (it->second & orbitFlags) != 0);
        lua_settable(l,-3);
        it++;
    }
    return 1;
}

static int celestia_setfaintestvisible(lua_State* l)
{
    checkArgs(l, 2, 2, "One argument expected for celestia:setfaintestvisible()");
    CelestiaCore* appCore = this_celestia(l);
    float faintest = (float)safeGetNumber(l, 2, AllErrors, "Argument to celestia:setfaintestvisible() must be a number");
    if ((appCore->getRenderer()->getRenderFlags() & Renderer::ShowAutoMag) == 0)
    {
        faintest = min(15.0f, max(1.0f, faintest));
        appCore->setFaintest(faintest);
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
    checkArgs(l, 1, 1, "No arguments expected for celestia:getfaintestvisible()");
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

static int celestia_setminfeaturesize(lua_State* l)
{
    checkArgs(l, 2, 2, "One argument expected for celestia:setminfeaturesize()");
    CelestiaCore* appCore = this_celestia(l);
    float minFeatureSize = (float)safeGetNumber(l, 2, AllErrors, "Argument to celestia:setminfeaturesize() must be a number");
    minFeatureSize = max(0.0f, minFeatureSize);
    appCore->getRenderer()->setMinimumFeatureSize(minFeatureSize);
    return 0;
}

static int celestia_getminfeaturesize(lua_State* l)
{
    checkArgs(l, 1, 1, "No arguments expected for celestia:getminfeaturesize()");
    CelestiaCore* appCore = this_celestia(l);
    lua_pushnumber(l, appCore->getRenderer()->getMinimumFeatureSize());
    return 1;
}

static int celestia_getobserver(lua_State* l)
{
    checkArgs(l, 1, 1, "No arguments expected for celestia:getobserver()");

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
    checkArgs(l, 1, 1, "No arguments expected for celestia:getobservers()");
    CelestiaCore* appCore = getAppCore(l, AllErrors);

    std::vector<Observer*> observer_list;
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
    checkArgs(l, 1, 1, "No arguments expected to celestia:getselection()");
    CelestiaCore* appCore = this_celestia(l);
    Selection sel = appCore->getSimulation()->getSelection();
    object_new(l, sel);

    return 1;
}

static int celestia_find(lua_State* l)
{
    checkArgs(l, 2, 2, "One argument expected for function celestia:find()");
    if (!lua_isstring(l, 2))
    {
        doError(l, "Argument to find must be a string");
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
    checkArgs(l, 2, 2, "One argument expected for celestia:select()");
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
    checkArgs(l, 2, 2, "One argument expected to function celestia:mark");

    CelestiaCore* appCore = this_celestia(l);
    Simulation* sim = appCore->getSimulation();
    Selection* sel = to_object(l, 2);

    if (sel != NULL)
    {
        sim->getUniverse()->markObject(*sel, 10.0f,
                                       Color(0.0f, 1.0f, 0.0f), Marker::Diamond, 1);
    }
    else
    {
        doError(l, "Argument to celestia:mark must be an object");
    }

    return 0;
}

static int celestia_unmark(lua_State* l)
{
    checkArgs(l, 2, 2, "One argument expected to function celestia:unmark");

    CelestiaCore* appCore = this_celestia(l);
    Simulation* sim = appCore->getSimulation();
    Selection* sel = to_object(l, 2);

    if (sel != NULL)
    {
        sim->getUniverse()->unmarkObject(*sel, 1);
    }
    else
    {
        doError(l, "Argument to celestia:unmark must be an object");
    }

    return 0;
}

static int celestia_gettime(lua_State* l)
{
    checkArgs(l, 1, 1, "No argument expected to function celestia:gettime");

    CelestiaCore* appCore = this_celestia(l);
    Simulation* sim = appCore->getSimulation();
    lua_pushnumber(l, sim->getTime());

    return 1;
}

static int celestia_gettimescale(lua_State* l)
{
    checkArgs(l, 1, 1, "No argument expected to function celestia:gettimescale");

    CelestiaCore* appCore = this_celestia(l);
    lua_pushnumber(l, appCore->getSimulation()->getTimeScale());

    return 1;
}

static int celestia_settime(lua_State* l)
{
    checkArgs(l, 2, 2, "One argument expected to function celestia:settime");

    CelestiaCore* appCore = this_celestia(l);
    double t = safeGetNumber(l, 2, AllErrors, "Second arg to celestia:settime must be a number");
    appCore->getSimulation()->setTime(t);

    return 0;
}

static int celestia_settimescale(lua_State* l)
{
    checkArgs(l, 2, 2, "One argument expected to function celestia:settimescale");

    CelestiaCore* appCore = this_celestia(l);
    double t = safeGetNumber(l, 2, AllErrors, "Second arg to celestia:settimescale must be a number");
    appCore->getSimulation()->setTimeScale(t);

    return 0;
}

static int celestia_tojulianday(lua_State* l)
{
    checkArgs(l, 2, 7, "Wrong number of arguments to function celestia:tojulianday");

    // for error checking only:
    this_celestia(l);

    int year = (int)safeGetNumber(l, 2, AllErrors, "First arg to celestia:tojulianday must be a number", 0.0);
    int month = (int)safeGetNumber(l, 3, WrongType, "Second arg to celestia:tojulianday must be a number", 1.0);
    int day = (int)safeGetNumber(l, 4, WrongType, "Third arg to celestia:tojulianday must be a number", 1.0);
    int hour = (int)safeGetNumber(l, 5, WrongType, "Fourth arg to celestia:tojulianday must be a number", 0.0);
    int minute = (int)safeGetNumber(l, 6, WrongType, "Fifth arg to celestia:tojulianday must be a number", 0.0);
    double seconds = safeGetNumber(l, 7, WrongType, "Sixth arg to celestia:tojulianday must be a number", 0.0);

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
    checkArgs(l, 2, 2, "Wrong number of arguments to function celestia:fromjulianday");

    // for error checking only:
    this_celestia(l);

    double jd = safeGetNumber(l, 2, AllErrors, "First arg to celestia:fromjulianday must be a number", 0.0);
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

static int celestia_unmarkall(lua_State* l)
{
    checkArgs(l, 1, 1, "No arguments expected to function celestia:unmarkall");

    CelestiaCore* appCore = this_celestia(l);
    Simulation* sim = appCore->getSimulation();
    sim->getUniverse()->unmarkAll();

    return 0;
}

static int celestia_getstarcount(lua_State* l)
{
    checkArgs(l, 1, 1, "No arguments expected to function celestia:getstarcount");

    CelestiaCore* appCore = this_celestia(l);
    Universe* u = appCore->getSimulation()->getUniverse();
    lua_pushnumber(l, u->getStarCatalog()->size());

    return 1;
}

static int celestia_setambient(lua_State* l)
{
    checkArgs(l, 2, 2, "One argument expected in celestia:setambient");
    CelestiaCore* appCore = this_celestia(l);

    Renderer* renderer = appCore->getRenderer();
    double ambientLightLevel = safeGetNumber(l, 2, AllErrors, "Argument to celestia:setambient must be a number");
    if (ambientLightLevel > 1.0)
        ambientLightLevel = 1.0;
    if (ambientLightLevel < 0.0)
        ambientLightLevel = 0.0;

    if (renderer != NULL)
        renderer->setAmbientLightLevel((float)ambientLightLevel);
    return 0;
}

static int celestia_getambient(lua_State* l)
{
    checkArgs(l, 1, 1, "No argument expected in celestia:setambient");
    CelestiaCore* appCore = this_celestia(l);

    Renderer* renderer = appCore->getRenderer();
    if (renderer == NULL)
    {
        doError(l, "Internal Error: renderer is NULL!");
    }
    else
    {
        lua_pushnumber(l, renderer->getAmbientLightLevel());
    }
    return 1;
}

static int celestia_setminorbitsize(lua_State* l)
{
    checkArgs(l, 2, 2, "One argument expected in celestia:setminorbitsize");
    CelestiaCore* appCore = this_celestia(l);

    double orbitSize = safeGetNumber(l, 2, AllErrors, "Argument to celestia:setminorbitsize() must be a number");
    Renderer* renderer = appCore->getRenderer();
    if (renderer == NULL)
    {
        doError(l, "Internal Error: renderer is NULL!");
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
    checkArgs(l, 1, 1, "No argument expected in celestia:getminorbitsize");
    CelestiaCore* appCore = this_celestia(l);

    Renderer* renderer = appCore->getRenderer();
    if (renderer == NULL)
    {
        doError(l, "Internal Error: renderer is NULL!");
    }
    else
    {
        lua_pushnumber(l, renderer->getMinimumOrbitSize());
    }
    return 1;
}

static int celestia_setstardistancelimit(lua_State* l)
{
    checkArgs(l, 2, 2, "One argument expected in celestia:setstardistancelimit");
    CelestiaCore* appCore = this_celestia(l);

    double distanceLimit = safeGetNumber(l, 2, AllErrors, "Argument to celestia:setstardistancelimit() must be a number");
    Renderer* renderer = appCore->getRenderer();
    if (renderer == NULL)
    {
        doError(l, "Internal Error: renderer is NULL!");
    }
    else
    {
        renderer->setDistanceLimit((float)distanceLimit);
    }
    return 0;
}

static int celestia_getstardistancelimit(lua_State* l)
{
    checkArgs(l, 1, 1, "No argument expected in celestia:getstardistancelimit");
    CelestiaCore* appCore = this_celestia(l);

    Renderer* renderer = appCore->getRenderer();
    if (renderer == NULL)
    {
        doError(l, "Internal Error: renderer is NULL!");
    }
    else
    {
        lua_pushnumber(l, renderer->getDistanceLimit());
    }
    return 1;
}

static int celestia_getstarstyle(lua_State* l)
{
    checkArgs(l, 1, 1, "No argument expected in celestia:getstarstyle");
    CelestiaCore* appCore = this_celestia(l);

    Renderer* renderer = appCore->getRenderer();
    if (renderer == NULL)
    {
        doError(l, "Internal Error: renderer is NULL!");
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
    checkArgs(l, 2, 2, "One argument expected in celestia:setstarstyle");
    CelestiaCore* appCore = this_celestia(l);

    string starStyle = safeGetString(l, 2, AllErrors, "Argument to celestia:setstarstyle must be a string");
    Renderer* renderer = appCore->getRenderer();
    if (renderer == NULL)
    {
        doError(l, "Internal Error: renderer is NULL!");
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
            doError(l, "Invalid starstyle");
        }
    }
    return 0;
}

static int celestia_getstar(lua_State* l)
{
    checkArgs(l, 2, 2, "One argument expected to function celestia:getstar");

    CelestiaCore* appCore = this_celestia(l);
    double starIndex = safeGetNumber(l, 2, AllErrors, "First arg to celestia:getstar must be a number");
    Universe* u = appCore->getSimulation()->getUniverse();
    Star* star = u->getStarCatalog()->getStar((uint32) starIndex);
    if (star == NULL)
        lua_pushnil(l);
    else
        object_new(l, Selection(star));

    return 1;
}

static int celestia_newvector(lua_State* l)
{
    checkArgs(l, 4, 4, "Expected 3 arguments for celestia:newvector");
    // for error checking only:
    this_celestia(l);
    double x = safeGetNumber(l, 2, AllErrors, "First arg to celestia:newvector must be a number");
    double y = safeGetNumber(l, 3, AllErrors, "Second arg to celestia:newvector must be a number");
    double z = safeGetNumber(l, 4, AllErrors, "Third arg to celestia:newvector must be a number");

    vector_new(l, Vec3d(x,y,z));

    return 1;
}

static int celestia_newposition(lua_State* l)
{
    checkArgs(l, 4, 4, "Expected 3 arguments for celestia:newposition");
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
            doError(l, "Arguments to celestia:newposition must be either numbers or strings");
        }
    }

    position_new(l, UniversalCoord(components[0], components[1], components[2]));

    return 1;
}

static int celestia_newrotation(lua_State* l)
{
    checkArgs(l, 3, 5, "Need 2 or 4 arguments for celestia:newrotation");
    // for error checking only:
    this_celestia(l);

    if (lua_gettop(l) > 3)
    {
        // if (lua_gettop == 4), safeGetNumber will catch the error
        double w = safeGetNumber(l, 2, AllErrors, "arguments to celestia:newrotation must either be (vec, number) or four numbers");
        double x = safeGetNumber(l, 3, AllErrors, "arguments to celestia:newrotation must either be (vec, number) or four numbers");
        double y = safeGetNumber(l, 4, AllErrors, "arguments to celestia:newrotation must either be (vec, number) or four numbers");
        double z = safeGetNumber(l, 5, AllErrors, "arguments to celestia:newrotation must either be (vec, number) or four numbers");
        Quatd q(w, x, y, z);
        rotation_new(l, q);
    }
    else
    {
        Vec3d* v = to_vector(l, 2);
        if (v == NULL)
        {
            doError(l, "newrotation: first argument must be a vector");
        }
        double angle = safeGetNumber(l, 3, AllErrors, "second argument to celestia:newrotation must be a number");
        Quatd q;
        q.setAxisAngle(*v, angle);
        rotation_new(l, q);
    }
    return 1;
}

static int celestia_getscripttime(lua_State* l)
{
    checkArgs(l, 1, 1, "No arguments expected for celestia:getscripttime");
    // for error checking only:
    this_celestia(l);

    LuaState* luastate_ptr = getLuaStateObject(l);
    lua_pushnumber(l, luastate_ptr->getTime());
    return 1;
}

static int celestia_newframe(lua_State* l)
{
    checkArgs(l, 2, 4, "One to three arguments expected for function celestia:newframe");
    int argc = lua_gettop(l);

    // for error checking only:
    this_celestia(l);

    const char* coordsysName = safeGetString(l, 2, AllErrors, "newframe: first argument must be a string");
    astro::CoordinateSystem coordSys = parseCoordSys(coordsysName);
    Selection* ref = NULL;
    Selection* target = NULL;

    if (coordSys == astro::Universal)
    {
        frame_new(l, FrameOfReference());
    }
    else if (coordSys == astro::PhaseLock)
    {
        if (argc >= 4)
        {
            ref = to_object(l, 3);
            target = to_object(l, 4);
        }

        if (ref == NULL || target == NULL)
        {
            doError(l, "newframe: two objects required for lock frame");
        }

        frame_new(l, FrameOfReference(coordSys, *ref, *target));
    }
    else
    {
        if (argc >= 3)
            ref = to_object(l, 3);
        if (ref == NULL)
        {
            doError(l, "newframe: one object argument required for frame");
        }

        frame_new(l, FrameOfReference(coordSys, *ref));
    }

    return 1;
}

static int celestia_requestkeyboard(lua_State* l)
{
    checkArgs(l, 2, 2, "Need one arguments for celestia:requestkeyboard");
    CelestiaCore* appCore = this_celestia(l);

    if (!lua_isboolean(l, 2))
    {
        doError(l, "First argument for celestia:requestkeyboard must be a boolean");
    }

    int mode = appCore->getTextEnterMode();

    if (lua_toboolean(l, 2))
    {
        // Check for existence of charEntered:
        lua_pushstring(l, KbdCallback);
        lua_gettable(l, LUA_GLOBALSINDEX);
        if (lua_isnil(l, -1))
        {
            doError(l, "script requested keyboard, but did not provide callback");
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

static int celestia_takescreenshot(lua_State* l)
{
    checkArgs(l, 1, 3, "Need 0 to 2 arguments for celestia:takescreenshot");
    CelestiaCore* appCore = this_celestia(l);
    LuaState* luastate = getLuaStateObject(l);
    // make sure we don't timeout because of taking a screenshot:
    double timeToTimeout = luastate->timeout - luastate->getTime();

    const char* filetype = safeGetString(l, 2, WrongType, "First argument to celestia:takescreenshot must be a string");
    if (filetype == NULL)
        filetype = "png";

    // Let the script safely contribute one part of the filename: 
    const char* fileid_ptr = safeGetString(l, 3, WrongType, "Second argument to celestia:takescreenshot must be a string");
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
    if (fileid.length() > 8)
        fileid = fileid.substr(0, 8);
    if (fileid.length() > 0)
        fileid.append("-");

    int maxCount = (int) appCore->getConfig()->scriptScreenshotCount;
    if ((luastate->screenshotCount < maxCount)  || (maxCount == -1))
    {
        string path = appCore->getConfig()->scriptScreenshotDirectory;
        if (path.length() > 0 && 
            path[path.length()-1] != '/' && 
            path[path.length()-1] != '\\')

            path.append("/");

            luastate->screenshotCount++;
        bool success = false;
        char filenamestem[32];
        sprintf(filenamestem, "screenshot-%s%06i", fileid.c_str(), luastate->screenshotCount);

        // Get the dimensions of the current viewport
        int viewport[4];
        glGetIntegerv(GL_VIEWPORT, viewport);

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
        lua_pushboolean(l, success);
    }
    else
    {
        lua_pushboolean(l, false);
    }
    // no matter how long it really took, make it look like 0.1s to timeout check:
    luastate->timeout = luastate->getTime() + timeToTimeout - 0.1;
    return 1;
}

static int celestia_createcelscript(lua_State* l)
{
    checkArgs(l, 2, 2, "Need one argument for celestia:createcelscript()");
    string scripttext = safeGetString(l, 2, AllErrors, "Argument to celestia:createcelscript() must be a string");
    return celscript_from_string(l, scripttext);
}

static int celestia_tostring(lua_State* l)
{
    lua_pushstring(l, "[Celestia]");

    return 1;
}

static void CreateCelestiaMetaTable(lua_State* l)
{
    CreateClassMetatable(l, _Celestia);

    RegisterMethod(l, "__tostring", celestia_tostring);
    RegisterMethod(l, "flash", celestia_flash);
    RegisterMethod(l, "print", celestia_print);
    RegisterMethod(l, "show", celestia_show);
    RegisterMethod(l, "hide", celestia_hide);
    RegisterMethod(l, "getrenderflags", celestia_getrenderflags);
    RegisterMethod(l, "setrenderflags", celestia_setrenderflags);
    RegisterMethod(l, "showlabel", celestia_showlabel);
    RegisterMethod(l, "hidelabel", celestia_hidelabel);
    RegisterMethod(l, "getlabelflags", celestia_getlabelflags);
    RegisterMethod(l, "setlabelflags", celestia_setlabelflags);
    RegisterMethod(l, "getorbitflags", celestia_getorbitflags);
    RegisterMethod(l, "setorbitflags", celestia_setorbitflags);
    RegisterMethod(l, "getfaintestvisible", celestia_getfaintestvisible);
    RegisterMethod(l, "setfaintestvisible", celestia_setfaintestvisible);
    RegisterMethod(l, "setminfeaturesize", celestia_setminfeaturesize);
    RegisterMethod(l, "getminfeaturesize", celestia_getminfeaturesize);
    RegisterMethod(l, "getobserver", celestia_getobserver);
    RegisterMethod(l, "getobservers", celestia_getobservers);
    RegisterMethod(l, "getselection", celestia_getselection);
    RegisterMethod(l, "find", celestia_find);
    RegisterMethod(l, "select", celestia_select);
    RegisterMethod(l, "mark", celestia_mark);
    RegisterMethod(l, "unmark", celestia_unmark);
    RegisterMethod(l, "unmarkall", celestia_unmarkall);
    RegisterMethod(l, "gettime", celestia_gettime);
    RegisterMethod(l, "settime", celestia_settime);
    RegisterMethod(l, "gettimescale", celestia_gettimescale);
    RegisterMethod(l, "settimescale", celestia_settimescale);
    RegisterMethod(l, "getambient", celestia_getambient);
    RegisterMethod(l, "setambient", celestia_setambient);
    RegisterMethod(l, "getminorbitsize", celestia_getminorbitsize);
    RegisterMethod(l, "setminorbitsize", celestia_setminorbitsize);
    RegisterMethod(l, "getstardistancelimit", celestia_getstardistancelimit);
    RegisterMethod(l, "setstardistancelimit", celestia_setstardistancelimit);
    RegisterMethod(l, "getstarstyle", celestia_getstarstyle);
    RegisterMethod(l, "setstarstyle", celestia_setstarstyle);
    RegisterMethod(l, "tojulianday", celestia_tojulianday);
    RegisterMethod(l, "fromjulianday", celestia_fromjulianday);
    RegisterMethod(l, "getstarcount", celestia_getstarcount);
    RegisterMethod(l, "getstar", celestia_getstar);
    RegisterMethod(l, "newframe", celestia_newframe);
    RegisterMethod(l, "newvector", celestia_newvector);
    RegisterMethod(l, "newposition", celestia_newposition);
    RegisterMethod(l, "newrotation", celestia_newrotation);
    RegisterMethod(l, "getscripttime", celestia_getscripttime);
    RegisterMethod(l, "requestkeyboard", celestia_requestkeyboard);
    RegisterMethod(l, "takescreenshot", celestia_takescreenshot);
    RegisterMethod(l, "createcelscript", celestia_createcelscript);

    lua_pop(l, 1);
}

// ==================== Initialization ====================
bool LuaState::init(CelestiaCore* appCore)
{
    initMaps();

    // Import the base and math libraries
    lua_baselibopen(state);
    lua_mathlibopen(state);
    lua_tablibopen(state);
    lua_strlibopen(state);

    // Add an easy to use wait function, so that script writers can
    // live in ignorance of coroutines.  There will probably be a significant
    // library of useful functions that can be defined purely in Lua.
    // At that point, we'll want something a bit more robust than just
    // parsing the whole text of the library every time a script is launched
    if (loadScript("wait = function(x) coroutine.yield(x) end") != 0)
        return false;
    lua_pcall(state, 0, 0, 0); // execute it

    lua_pushstring(state, "KM_PER_MICROLY");
    lua_pushnumber(state, (lua_Number)KM_PER_LY/1e6);
    lua_settable(state, LUA_GLOBALSINDEX);

    CreateObjectMetaTable(state);
    CreateObserverMetaTable(state);
    CreateCelestiaMetaTable(state);
    CreatePositionMetaTable(state);
    CreateVectorMetaTable(state);
    CreateRotationMetaTable(state);
    CreateFrameMetaTable(state);
    CreateCelscriptMetaTable(state);

    // Create the celestia object
    lua_pushstring(state, "celestia");
    celestia_new(state, appCore);
    lua_settable(state, LUA_GLOBALSINDEX);
    // add a reference to the LuaState-object in the registry
    lua_pushstring(state, "celestia-luastate");
    lua_pushlightuserdata(state, static_cast<void*>(this));
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
