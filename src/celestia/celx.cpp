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
#include <celengine/celestia.h>

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
};

static const int _Celestia = 0;
static const int _Observer = 1;
static const int _Object   = 2;
static const int _Vec3     = 3;
static const int _Matrix   = 4;
static const int _Rotation = 5;
static const int _Position = 6;
static const int _Frame    = 7;

#define CLASS(i) ClassNames[(i)]


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
static void* CheckUserData(lua_State* l, int index, int id)
{
    // get registry[metatable]
    if (!lua_getmetatable(l, index))
        return NULL;
    lua_rawget(l, LUA_REGISTRYINDEX);

    if (lua_type(l, -1) != LUA_TSTRING)
    {
        cout << "CheckUserData failed!  Unregistered class.\n";
        lua_pop(l, 1);
        return NULL;
    }

    const char* classname = lua_tostring(l, -1);
    if (classname != NULL && strcmp(classname, ClassNames[id]) == 0)
    {
        lua_pop(l, 1);
        return lua_touserdata(l, index);
    }
    else
    {
        cout << "CheckUserData failed!  Expected " << ClassNames[id] << " but got " << classname << '\n';
        lua_pop(l, 1);
        return NULL;
    }
}


LuaState::LuaState() :
    state(NULL),
    costate(NULL),
    alive(false)
{
    state = lua_open();
}

LuaState::~LuaState()
{
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


static void checkArgs(lua_State* l,
                      int minArgs, int maxArgs, char* errorMessage)
{
    int argc = lua_gettop(l);
    if (argc < minArgs || argc > maxArgs)
    {
        lua_pushstring(l, errorMessage);
        lua_error(l);
    }
}


static int parseRenderFlag(const string& name)
{
    if (compareIgnoringCase(name, "orbits") == 0)
        return Renderer::ShowOrbits;
    else if (compareIgnoringCase(name, "cloudmaps") == 0)
        return Renderer::ShowCloudMaps;
    else if (compareIgnoringCase(name, "constellations") == 0)
        return Renderer::ShowDiagrams;
    else if (compareIgnoringCase(name, "galaxies") == 0)
        return Renderer::ShowGalaxies;
    else if (compareIgnoringCase(name, "planets") == 0)
        return Renderer::ShowPlanets;
    else if (compareIgnoringCase(name, "stars") == 0)
        return Renderer::ShowStars;
    else if (compareIgnoringCase(name, "nightmaps") == 0)
        return Renderer::ShowNightMaps;
    else if (compareIgnoringCase(name, "eclipseshadows") == 0)
        return Renderer::ShowEclipseShadows;
    else if (compareIgnoringCase(name, "ringshadows") == 0)
        return Renderer::ShowRingShadows;
    else if (compareIgnoringCase(name, "pointstars") == 0)
        return Renderer::ShowStarsAsPoints;
    else if (compareIgnoringCase(name, "ringshadows") == 0)
        return Renderer::ShowRingShadows;
    else if (compareIgnoringCase(name, "comettails") == 0)
        return Renderer::ShowCometTails;
    else if (compareIgnoringCase(name, "boundaries") == 0)
        return Renderer::ShowBoundaries;
    else if (compareIgnoringCase(name, "markers") == 0)
        return Renderer::ShowMarkers;
    else
        return 0;
}


static int parseLabelFlag(const string& name)
{
    if (compareIgnoringCase(name, "planets") == 0)
        return Renderer::PlanetLabels;
    else if (compareIgnoringCase(name, "moons") == 0)
        return Renderer::MoonLabels;
    else if (compareIgnoringCase(name, "spacecraft") == 0)
        return Renderer::SpacecraftLabels;
    else if (compareIgnoringCase(name, "asteroids") == 0)
        return Renderer::AsteroidLabels;
    else if (compareIgnoringCase(name, "constellations") == 0)
        return Renderer::ConstellationLabels;
    else if (compareIgnoringCase(name, "stars") == 0)
        return Renderer::StarLabels;
    else if (compareIgnoringCase(name, "galaxies") == 0)
        return Renderer::GalaxyLabels;
    else
        return 0;
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


static Color to_color(lua_State* l, int index)
{
    Color c(0.0f, 0.0f, 0.0f);

    if (lua_isstring(l, index))
    {
        const char* s = lua_tostring(l, index);
        Color::parse(s, c);
    }

    return c;
}


// Return the CelestiaCore object stored in the globals table
static CelestiaCore* getAppCore(lua_State* l)
{
    lua_pushstring(l, "celestia");
    lua_gettable(l, LUA_GLOBALSINDEX);
    CelestiaCore** appCore =
        static_cast<CelestiaCore**>(CheckUserData(l, -1, _Celestia));
    lua_pop(l, 1);

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
    assert(co == costate);
    if (co != costate)
        return false;

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


// position - a 128-bit per component universal coordinate
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
        lua_pushstring(l, "Bad position object!");
        lua_error(l);
    }

    return uc;
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
        lua_pushstring(l, "Position expected as argument to position:distanceto");
        lua_error(l);
    }

    Vec3d v = *uc2 - *uc;
    lua_pushnumber(l, astro::microLightYearsToKilometers(v.length()));
        
    return 1;
}

static void CreatePositionMetaTable(lua_State* l)
{
    CreateClassMetatable(l, _Position);
    
    RegisterMethod(l, "__tostring", position_tostring);
    RegisterMethod(l, "distanceto", position_distanceto);

    lua_pop(l, 1); // remove metatable from stack
}


// frame of reference
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

// Convert from frame coordinates to universal.  The arguments are a time
// (Julian day number), position, and optionally an orientation.  If just
// a position is passed, the function returns just the converted position.
// Otherwise, the transformed position and orientation are both pushed onto
// the stack.
static int frame_from(lua_State* l)
{
    int argc = lua_gettop(l);
    if (argc < 3)
    {
        lua_pushstring(l, "Two arguments expected to function frame:from");
        lua_error(l);
    }

    FrameOfReference* frame = to_frame(l, 1);
    if (frame != NULL)
    {
        // Time is the first argument
        if (!lua_isnumber(l, 2))
        {
            lua_pushstring(l, "Time expected as first argument to frame:from()");
            lua_error(l);
        }
        double jd = lua_tonumber(l, 2);

        RigidTransform rt;
        UniversalCoord* uc = to_position(l, 3);
        if (uc != NULL)
        {
            rt.translation = *uc;
        }
        else
        {
            lua_pushstring(l, "Position expected as second argument to frame:from()");
            lua_error(l);
        }

        if (argc > 3)
        {
            // handle optional orientation
        }

        rt = frame->toUniversal(rt, jd);
        position_new(l, rt.translation);
    }
    else
    {
        lua_pushstring(l, "Bad method call");
        lua_error(l);
    }

    return 1;
}


// Convert from universal to frame coordinates.  The arguments are a time
// (Julian day number), position, and optionally an orientation.  If just
// a position is passed, the function returns just the converted position.
// Otherwise, the transformed position and orientation are both pushed onto
// the stack.
static int frame_to(lua_State* l)
{
    int argc = lua_gettop(l);
    if (argc < 3)
    {
        lua_pushstring(l, "Two arguments expected to function frame:to");
        lua_error(l);
    }

    FrameOfReference* frame = to_frame(l, 1);
    if (frame != NULL)
    {
        // Time is the first argument
        if (!lua_isnumber(l, 2))
        {
            lua_pushstring(l, "Time expected as first argument to frame:to()");
            lua_error(l);
        }
        double jd = lua_tonumber(l, 2);

        RigidTransform rt;
        UniversalCoord* uc = to_position(l, 3);
        if (uc != NULL)
        {
            rt.translation = *uc;
        }
        else
        {
            lua_pushstring(l, "Position expected as second argument to frame:to()");
            lua_error(l);
        }

        if (argc > 3)
        {
            // handle optional orientation
        }

        rt = frame->fromUniversal(rt, jd);
        position_new(l, rt.translation);
    }
    else
    {
        lua_pushstring(l, "Bad method call");
        lua_error(l);
    }

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

    lua_pop(l, 1); // remove metatable from stack
}


// object - star, planet, or deep-sky object
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
        lua_pushstring(l, "Bad position object!");
        lua_error(l);
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
    int argc = lua_gettop(l);
    if (argc != 1)
    {
        lua_pushstring(l, "No arguments expected to function object:radius");
        lua_error(l);
    }

    Selection* sel = to_object(l, 1);
    if (sel != NULL)
    {
        lua_pushnumber(l, sel->radius());
    }
    else
    {
        lua_pushstring(l, "Bad object!");
        lua_error(l);
    }

    return 1;
}

static int object_type(lua_State* l)
{
    int argc = lua_gettop(l);
    if (argc != 1)
    {
        lua_pushstring(l, "No arguments expected to function object:type");
        lua_error(l);
    }

    Selection* sel = to_object(l, 1);
    if (sel != NULL)
    {
        char* tname = "unknown";
        if (sel->body != NULL)
        {
            int cl = sel->body->getClassification();
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
        else if (sel->star != NULL)
        {
            tname = "star";
        }
        else if (sel->deepsky != NULL)
        {
            // TODO: return cluster, galaxy, or nebula as appropriate
            tname = "deepsky";
        }
            
        lua_pushstring(l, tname);
    }
    else
    {
        lua_pushstring(l, "Bad object!");
        lua_error(l);
    }

    return 1;
}


static int object_name(lua_State* l)
{
    int argc = lua_gettop(l);
    if (argc != 1)
    {
        lua_pushstring(l, "No arguments expected to function object:name");
        lua_error(l);
    }

    Selection* sel = to_object(l, 1);
    if (sel != NULL)
    {
        if (sel->body != NULL)
        {
            lua_pushstring(l, sel->body->getName().c_str());
        }
        else if (sel->deepsky != NULL)
        {
            lua_pushstring(l, sel->deepsky->getName().c_str());
        }
        else
        {
            // TODO: look up the real star name
            lua_pushstring(l, "[fix to handle stars]");
        }
    }
    else
    {
        lua_pushstring(l, "Bad object!");
        lua_error(l);
    }

    return 1;
}


static int object_spectraltype(lua_State* l)
{
    int argc = lua_gettop(l);
    if (argc != 1)
    {
        lua_pushstring(l, "No arguments expected to function object:spectraltype");
        lua_error(l);
    }

    Selection* sel = to_object(l, 1);
    if (sel != NULL)
    {
        if (sel->star != NULL)
        {
            char buf[16];
            StellarClass sc = sel->star->getStellarClass();
            if (sc.str(buf, sizeof(buf)))
            {
                lua_pushstring(l, buf);
            }
            else
            {
                // This should only happen if the spectral type has > 15 chars
                // (i.e. never, unless there's a bug)
                assert(0);
                lua_pushstring(l, "Bad spectral type (this is a bug!)");
                lua_error(l);
            }
        }
        else
        {
            lua_pushnil(l);
        }
    }
    else
    {
        lua_pushstring(l, "Bad object!");
        lua_error(l);
    }

    return 1;
}


static int object_absmag(lua_State* l)
{
    int argc = lua_gettop(l);
    if (argc != 1)
    {
        lua_pushstring(l, "No arguments expected to function object:absmag");
        lua_error(l);
    }

    Selection* sel = to_object(l, 1);
    if (sel != NULL)
    {
        if (sel->star != NULL)
            lua_pushnumber(l, sel->star->getAbsoluteMagnitude());
        else
            lua_pushnil(l);
    }
    else
    {
        lua_pushstring(l, "Bad object!");
        lua_error(l);
    }

    return 1;
}


static int object_mark(lua_State* l)
{
    int argc = lua_gettop(l);
    if (argc < 1)
    {
        lua_pushstring(l, "Bad call to object:mark");
        lua_error(l);
    }

    Selection* sel = to_object(l, 1);
    if (sel != NULL)
    {
        Color markColor(0.0f, 1.0f, 0.0f);
        if (argc > 1)
            markColor = to_color(l, 2);
        
        Marker::Symbol markSymbol = Marker::Diamond;
        if (argc > 2 && lua_isstring(l, 3))
            markSymbol = parseMarkerSymbol(lua_tostring(l, 3));

        float markSize = 10.0f;
        if (argc > 3 && lua_isnumber(l, 4))
        {
            markSize = (float) lua_tonumber(l, 4);
            if (markSize < 1.0f)
                markSize = 1.0f;
            else if (markSize > 100.0f)
                markSize = 100.0f;
        }

        CelestiaCore* appCore = getAppCore(l);
        if (appCore != NULL)
        {
            Simulation* sim = appCore->getSimulation();
            sim->getUniverse()->markObject(*sel, markSize,
                                           markColor, markSymbol, 1);
        }
    }
    else
    {
        lua_pushstring(l, "Bad object!");
        lua_error(l);
    }

    return 0;
}


static int object_unmark(lua_State* l)
{
    int argc = lua_gettop(l);
    if (argc != 1)
    {
        lua_pushstring(l, "No arguments expected to function object:unmark");
        lua_error(l);
    }

    Selection* sel = to_object(l, 1);
    if (sel != NULL)
    {
        CelestiaCore* appCore = getAppCore(l);
        if (appCore != NULL)
        {
            Simulation* sim = appCore->getSimulation();
            sim->getUniverse()->unmarkObject(*sel, 1);
        }
    }
    else
    {
        lua_pushstring(l, "Bad object!");
        lua_error(l);
    }

    return 0;
}


// Return the observer's current position.  A time argument is optional;
// if not provided, the current master simulation time is used.
static int object_getposition(lua_State* l)
{
    int argc = lua_gettop(l);
    if (argc != 1 && argc != 2)
    {
        lua_pushstring(l, "Bad number of arguments to observer:getposition");
        lua_error(l);
    }

    Selection* sel = to_object(l, 1);
    if (sel != NULL)
    {
        double t = 0.0;
        if (argc == 2)
        {
            if (lua_isnumber(l, 2))
            {
                t = lua_tonumber(l, 2);
            }
            else
            {
                lua_pushstring(l, "Time expected as argument to object:getposition");
                lua_error(l);
            }
        }
        else
        {
            CelestiaCore* appCore = getAppCore(l);
            if (appCore != NULL)
                t = appCore->getSimulation()->getTime();
        }

        position_new(l, sel->getPosition(t));
    }
    else
    {
        lua_pushstring(l, "Bad object!");
        lua_error(l);
    }

    return 1;
}


static int object_getchildren(lua_State* l)
{
    checkArgs(l, 1, 1, "No arguments expected for object:getchildren()");
    Selection* sel = this_object(l);

    CelestiaCore* appCore = getAppCore(l);
    if (appCore == NULL)
    {
        lua_pushstring(l, "Celestia instance missing!");
        lua_error(l);
    }

    Simulation* sim = appCore->getSimulation();

    lua_newtable(l);
    if (sel->star != NULL)
    {
        SolarSystemCatalog* solarSystemCatalog = sim->getUniverse()->getSolarSystemCatalog();
        SolarSystemCatalog::iterator iter = solarSystemCatalog->find(sel->star->getCatalogNumber());
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
    else if (sel->body != NULL)
    {
        const PlanetarySystem* satellites = sel->body->getSatellites();
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


static void CreateObjectMetaTable(lua_State* l)
{
    CreateClassMetatable(l, _Object);

    RegisterMethod(l, "__tostring", object_tostring);
    RegisterMethod(l, "radius", object_radius);
    RegisterMethod(l, "type", object_type);
    RegisterMethod(l, "spectraltype", object_spectraltype);
    RegisterMethod(l, "absmag", object_absmag);
    RegisterMethod(l, "name", object_name);
    RegisterMethod(l, "mark", object_mark);
    RegisterMethod(l, "unmark", object_unmark);
    RegisterMethod(l, "getposition", object_getposition);
    RegisterMethod(l, "getchildren", object_getchildren);

    lua_pop(l, 1); // pop metatable off the stack
}


// observer object

static int observer_new(lua_State* l, Observer* o)
{
    Observer** ud = static_cast<Observer**>(lua_newuserdata(l, sizeof(Observer*)));
    *ud = o;

    SetClass(l, _Observer);

    return 1;
}


static Observer* to_observer(lua_State* l, int index)
{
    // TODO: need to verify that this is actually an object, not some
    // other userdata
    Observer** o = static_cast<Observer**>(lua_touserdata(l, index));
    if (o == NULL)
        return NULL;
    else
        return *o;
}


static int observer_tostring(lua_State* l)
{
    cout << "observer_tostring\n"; cout.flush();
    lua_pushstring(l, "[Observer]");

    return 1;
}


// First argument is the target object or position; optional second argument
// is the travel time
static int observer_goto(lua_State* l)
{
    int argc = lua_gettop(l);
    if (argc < 2)
    {
        lua_pushstring(l, "At least one argument expected to function observer:goto");
        lua_error(l);
    }

    double travelTime = 5.0;
    if (argc >= 3)
    {
        if (lua_isnumber(l, 3))
            travelTime = lua_tonumber(l, 3);
    }

    Observer* o = to_observer(l, 1);
    if (o != NULL)
    {
        // The first argument may be either an object or a position
        Selection* sel = to_object(l, 2);
        if (sel != NULL)
        {
            o->gotoSelection(*sel, travelTime, Vec3f(0, 1, 0),
                             astro::ObserverLocal);
        }
        else 
        {
            // TODO: would it be better to have a separate gotolocation
            // command?  Probably.
            UniversalCoord* uc = to_position(l, 2);
            if (uc != NULL)
            {
                RigidTransform rt = o->getSituation();
                rt.translation = *uc;
                o->gotoLocation(rt, travelTime);
            }
        }
    }

    return 0;
}


static int observer_center(lua_State* l)
{
    int argc = lua_gettop(l);
    if (argc < 2)
    {
        lua_pushstring(l, "At least one argument expected to function observer:center");
        lua_error(l);
    }

    double travelTime = 5.0;
    if (argc >= 3)
    {
        if (lua_isnumber(l, 3))
            travelTime = lua_tonumber(l, 3);
    }

    Observer* o = to_observer(l, 1);
    if (o != NULL)
    {
        Selection* sel = to_object(l, 2);
        if (sel != NULL)
            o->centerSelection(*sel, travelTime);
    }

    return 0;
}


static int observer_follow(lua_State* l)
{
    int argc = lua_gettop(l);
    if (argc != 2)
    {
        lua_pushstring(l, "One argument expected to function observer:follow");
        lua_error(l);
    }

    Observer* o = to_observer(l, 1);
    if (o != NULL)
    {
        Selection* sel = to_object(l, 2);
        if (sel != NULL)
            o->follow(*sel);
    }

    return 0;
}


static int observer_synchronous(lua_State* l)
{
    int argc = lua_gettop(l);
    if (argc != 2)
    {
        lua_pushstring(l, "One argument expected to function observer:synchronous");
        lua_error(l);
    }

    Observer* o = to_observer(l, 1);
    if (o != NULL)
    {
        Selection* sel = to_object(l, 2);
        if (sel != NULL)
            o->geosynchronousFollow(*sel);
    }

    return 0;
}


static int observer_lock(lua_State* l)
{
    int argc = lua_gettop(l);
    if (argc != 2)
    {
        lua_pushstring(l, "One argument expected to function observer:lock");
        lua_error(l);
    }

    Observer* o = to_observer(l, 1);
    if (o != NULL)
    {
        Selection* sel = to_object(l, 2);
        if (sel != NULL)
            o->phaseLock(*sel);
    }

    return 0;
}


static int observer_chase(lua_State* l)
{
    int argc = lua_gettop(l);
    if (argc != 2)
    {
        lua_pushstring(l, "One argument expected to function observer:chase");
        lua_error(l);
    }

    Observer* o = to_observer(l, 1);
    if (o != NULL)
    {
        Selection* sel = to_object(l, 2);
        if (sel != NULL)
            o->chase(*sel);
    }

    return 0;
}


static int observer_track(lua_State* l)
{
    int argc = lua_gettop(l);
    if (argc != 2)
    {
        lua_pushstring(l, "One argument expected to function observer:chase");
        lua_error(l);
    }

    Observer* o = to_observer(l, 1);
    if (o != NULL)
    {
        // If the argument is nil, clear the tracked object
        if (lua_isnil(l, 2))
        {
            o->setTrackedObject(Selection());
        }
        else
        {
            // Otherwise, turn on tracking and set the tracked object
            Selection* sel = to_object(l, 2);
            if (sel != NULL)
                o->setTrackedObject(*sel);
        }
    }

    return 0;
}


// Return true if the observer is still moving as a result of a goto, center,
// or similar command.
static int observer_travelling(lua_State* l)
{
    int argc = lua_gettop(l);
    if (argc != 1)
    {
        lua_pushstring(l, "No arguments expected to function observer:travelling");
        lua_error(l);
    }

    Observer* o = to_observer(l, 1);
    if (o != NULL)
    {
        if (o->getMode() == Observer::Travelling)
            lua_pushboolean(l, 1);
        else
            lua_pushboolean(l, 0);
    }
    else
    {
        lua_pushstring(l, "Bad method call");
        lua_error(l);
    }

    return 1;
}


// Return the observer's current time as a Julian day number
static int observer_gettime(lua_State* l)
{
    int argc = lua_gettop(l);
    if (argc != 1)
    {
        lua_pushstring(l, "No arguments expected to function observer:time");
        lua_error(l);
    }

    Observer* o = to_observer(l, 1);
    if (o != NULL)
    {
        lua_pushnumber(l, o->getTime());
    }
    else
    {
        lua_pushstring(l, "Bad method call");
        lua_error(l);
    }

    return 1;
}


// Return the observer's current position
static int observer_getposition(lua_State* l)
{
    int argc = lua_gettop(l);
    if (argc != 1)
    {
        lua_pushstring(l, "No arguments expected to function observer:getposition");
        lua_error(l);
    }

    Observer* o = to_observer(l, 1);
    if (o != NULL)
    {
        position_new(l, o->getPosition());
    }
    else
    {
        lua_pushstring(l, "Bad method call");
        lua_error(l);
    }

    return 1;
}


static void CreateObserverMetaTable(lua_State* l)
{
    CreateClassMetatable(l, _Observer);
    
    RegisterMethod(l, "__tostring", observer_tostring);
    RegisterMethod(l, "goto", observer_goto);
    RegisterMethod(l, "center", observer_center);
    RegisterMethod(l, "follow", observer_follow);
    RegisterMethod(l, "synchronous", observer_synchronous);
    RegisterMethod(l, "chase", observer_chase);
    RegisterMethod(l, "lock", observer_lock);
    RegisterMethod(l, "track", observer_track);
    RegisterMethod(l, "travelling", observer_travelling);
    RegisterMethod(l, "gettime", observer_gettime);
    RegisterMethod(l, "getposition", observer_getposition);

    lua_pop(l, 1); // remove metatable from stack
}


// Celestia objects

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
        lua_pushstring(l, "Bad celestia object!");
        lua_error(l);
    }

    return appCore;
}

static int celestia_flash(lua_State* l)
{
    checkArgs(l, 2, 2, "One argument expected to function celestia:flash");
    CelestiaCore* appCore = this_celestia(l);

    const char* s = lua_tostring(l, 2);

    if (appCore != NULL && s != NULL)
        appCore->flash(s, 1.5);

    return 0;
}


static int celestia_show(lua_State* l)
{
    checkArgs(l, 1, 1000, "Bad method call!");
    CelestiaCore* appCore = this_celestia(l);

    int argc = lua_gettop(l);
    int flags = 0;
    for (int i = 2; i <= argc; i++)
    {
        const char* s = lua_tostring(l, i);
        if (s != NULL)
            flags |= parseRenderFlag(s);
    }

    Renderer* r = appCore->getRenderer();
    r->setRenderFlags(r->getRenderFlags() | flags);

    return 0;
}


static int celestia_hide(lua_State* l)
{
    checkArgs(l, 1, 1000, "Bad method call!");
    CelestiaCore* appCore = this_celestia(l);

    int argc = lua_gettop(l);
    int flags = 0;
    for (int i = 2; i <= argc; i++)
    {
        const char* s = lua_tostring(l, i);
        if (s != NULL)
            flags |= parseRenderFlag(s);
    }

    Renderer* r = appCore->getRenderer();
    r->setRenderFlags(r->getRenderFlags() & ~flags);

    return 0;
}


static int celestia_showlabel(lua_State* l)
{
    checkArgs(l, 1, 1000, "Bad method call!");
    CelestiaCore* appCore = this_celestia(l);

    int argc = lua_gettop(l);
    int flags = 0;
    for (int i = 2; i <= argc; i++)
    {
        const char* s = lua_tostring(l, i);
        if (s != NULL)
            flags |= parseLabelFlag(s);
    }

    Renderer* r = appCore->getRenderer();
    r->setLabelMode(r->getLabelMode() | flags);

    return 0;
}


static int celestia_hidelabel(lua_State* l)
{
    checkArgs(l, 1, 1000, "Bad method call!");
    CelestiaCore* appCore = this_celestia(l);

    int argc = lua_gettop(l);
    int flags = 0;
    for (int i = 2; i <= argc; i++)
    {
        const char* s = lua_tostring(l, i);
        if (s != NULL)
            flags |= parseLabelFlag(s);
    }

    Renderer* r = appCore->getRenderer();
    r->setLabelMode(r->getLabelMode() & ~flags);

    return 0;
}


static int celestia_getobserver(lua_State* l)
{
    checkArgs(l, 1, 2, "Wrong number of arguments to celestia:getobserver()");

    CelestiaCore* appCore = this_celestia(l);
    Observer* o = appCore->getSimulation()->getActiveObserver();
    if (o == NULL)
        lua_pushnil(l);
    else
        observer_new(l, o);

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
    int argc = lua_gettop(l);
    if (argc != 2 || !lua_isstring(l, 2))
    {
        lua_pushstring(l, "One string argument expected for function celestia:find()");
        lua_error(l);
    }

    CelestiaCore* appCore = this_celestia(l);
    if (appCore != NULL)
    {
        Simulation* sim = appCore->getSimulation();
        // Should use universe not simulation for finding objects
        Selection sel = sim->findObjectFromPath(lua_tostring(l, 2));
        object_new(l, sel);
    }

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
    int argc = lua_gettop(l);
    if (argc != 2)
    {
        lua_pushstring(l, "One argument expected to function celestia:mark");
        lua_error(l);
    }

    CelestiaCore* appCore = to_celestia(l, 1);
    if (appCore != NULL)
    {
        Simulation* sim = appCore->getSimulation();
        Selection* sel = to_object(l, 2);

        if (sel != NULL)
        {
            sim->getUniverse()->markObject(*sel, 10.0f,
                                           Color(0.0f, 1.0f, 0.0f), Marker::Diamond, 1);
        }
    }

    return 0;
}


static int celestia_unmark(lua_State* l)
{
    int argc = lua_gettop(l);
    if (argc != 2)
    {
        lua_pushstring(l, "One argument expected to function celestia:unmark");
        lua_error(l);
    }

    CelestiaCore* appCore = to_celestia(l, 1);
    if (appCore != NULL)
    {
        Simulation* sim = appCore->getSimulation();
        Selection* sel = to_object(l, 2);

        if (sel != NULL)
            sim->getUniverse()->unmarkObject(*sel, 1);
    }

    return 0;
}


static int celestia_gettime(lua_State* l)
{
    int argc = lua_gettop(l);
    if (argc != 1)
    {
        lua_pushstring(l, "No argument expected to function celestia:gettime");
        lua_error(l);
    }

    CelestiaCore* appCore = to_celestia(l, 1);
    if (appCore != NULL)
    {
        Simulation* sim = appCore->getSimulation();
        lua_pushnumber(l, sim->getTime());
    }

    return 1;
}


static int celestia_gettimescale(lua_State* l)
{
    int argc = lua_gettop(l);
    if (argc != 1)
    {
        lua_pushstring(l, "No argument expected to function celestia:gettimescale");
        lua_error(l);
    }

    CelestiaCore* appCore = to_celestia(l, 1);
    if (appCore != NULL)
        lua_pushnumber(l, appCore->getSimulation()->getTimeScale());

    return 1;
}


static int celestia_settime(lua_State* l)
{
    int argc = lua_gettop(l);
    if (argc != 2)
    {
        lua_pushstring(l, "One argument expected to function celestia:settime");
        lua_error(l);
    }

    CelestiaCore* appCore = to_celestia(l, 1);
    if (appCore != NULL)
    {
        if (lua_isnumber(l, 2))
            appCore->getSimulation()->setTime(lua_tonumber(l, 2));
    }

    return 0;
}


static int celestia_settimescale(lua_State* l)
{
    int argc = lua_gettop(l);
    if (argc != 2)
    {
        lua_pushstring(l, "One argument expected to function celestia:settimescale");
        lua_error(l);
    }

    CelestiaCore* appCore = to_celestia(l, 1);
    if (appCore != NULL)
    {
        if (lua_isnumber(l, 2))
            appCore->getSimulation()->setTimeScale(lua_tonumber(l, 2));
    }

    return 0;
}


static int celestia_tojulianday(lua_State* l)
{
    int argc = lua_gettop(l);
    if (argc < 2 || argc > 7)
    {
        lua_pushstring(l, "Wrong number of arguments to function celestia:tojulianday");
        lua_error(l);
    }

    CelestiaCore* appCore = to_celestia(l, 1);
    if (appCore != NULL)
    {
        int year = 0;
        int month = 1;
        int day = 1;
        int hour = 0;
        int minute = 0;
        double seconds = 0.0;

        // Require that all arguments are numeric
        for (int i = 2; i < argc; i++)
        {
            if (!lua_isnumber(l, i))
            {
                lua_pushstring(l, "Arguments of celestia:tojulianday must be numberic");
                lua_error(l);
            }
        }

        year = (int) lua_tonumber(l, 2);
        if (argc > 2)
            month = (int) lua_tonumber(l, 3);
        if (argc > 3)
            day = (int) lua_tonumber(l, 4);
        if (argc > 4)
            hour = (int) lua_tonumber(l, 5);
        if (argc > 5)
            minute = (int) lua_tonumber(l, 6);
        if (argc > 6)
            seconds = lua_tonumber(l, 7);

        astro::Date date(year, month, day);
        date.hour = hour;
        date.minute = minute;
        date.seconds = seconds;

        double jd = (double) date;

        lua_pushnumber(l, jd);
    }

    return 1;
}


static int celestia_unmarkall(lua_State* l)
{
    checkArgs(l, 1, 1, "No arguments expected to function celestia:unmarkall");

    CelestiaCore* appCore = to_celestia(l, 1);
    if (appCore != NULL)
    {
        Simulation* sim = appCore->getSimulation();
        sim->getUniverse()->unmarkAll();
    }

    return 0;
}


static int celestia_getstarcount(lua_State* l)
{
    checkArgs(l, 1, 1, "No arguments expected to function celestia:getstarcount");

    CelestiaCore* appCore = to_celestia(l, 1);
    if (appCore != NULL)
    {
        Universe* u = appCore->getSimulation()->getUniverse();
        lua_pushnumber(l, u->getStarCatalog()->size());
    }
    else
    {
        lua_pushstring(l, "Bad celestia object");
        lua_error(l);
    }

    return 1;
}


static int celestia_getstar(lua_State* l)
{
    int argc = lua_gettop(l);
    if (argc != 2)
    {
        lua_pushstring(l, "One argument expected to function celestia:getstar");
        lua_error(l);
    }

    CelestiaCore* appCore = to_celestia(l, 1);
    if (appCore != NULL)
    {
        if (lua_isnumber(l, 2))
        {
            double starIndex = lua_tonumber(l, 2);
            Universe* u = appCore->getSimulation()->getUniverse();
            Star* star = u->getStarCatalog()->getStar((uint32) starIndex);
            if (star == NULL)
                lua_pushnil(l);
            else
                object_new(l, Selection(star));
        }
        else
        {
            lua_pushnil(l);
        }
    }
    else
    {
        lua_pushstring(l, "Bad celestia object");
        lua_error(l);
    }

    return 1;
}


static int celestia_newframe(lua_State* l)
{
    int argc = lua_gettop(l);
    if (argc < 2)
    {
        lua_pushstring(l, "At least argument expected to function celestia:getstar");
        lua_error(l);
    }

    CelestiaCore* appCore = to_celestia(l, 1);
    if (appCore != NULL)
    {
        if (!lua_isstring(l, 2))
        {
            lua_pushstring(l, "newframe: first argument must be a string");
            lua_error(l);
        }
        else
        {
            const char* coordsysName = lua_tostring(l, 2);
            astro::CoordinateSystem coordSys = parseCoordSys(coordsysName);
            Selection* ref = NULL;
            Selection* target = NULL;

            if (coordSys == astro::PhaseLock)
            {
                if (argc >= 4)
                {
                    ref = to_object(l, 3);
                    target = to_object(l, 4);
                }

                if (ref == NULL || target == NULL)
                {
                    lua_pushstring(l, "newframe: two objects required for lock frame");
                    lua_error(l);
                }
            }
            else
            {
                if (argc >= 3)
                    ref = to_object(l, 3);
                if (ref == NULL)
                {
                    lua_pushstring(l, "newframe: one object argument required for frame");
                    lua_error(l);
                }
            }
            
            frame_new(l, FrameOfReference(coordSys, *ref, *target));
        }
    }
    else
    {
        lua_pushstring(l, "Bad celestia object");
        lua_error(l);
    }

    return 1;
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
    RegisterMethod(l, "show", celestia_show);
    RegisterMethod(l, "hide", celestia_hide);
    RegisterMethod(l, "showlabel", celestia_showlabel);
    RegisterMethod(l, "hidelabel", celestia_hidelabel);
    RegisterMethod(l, "getobserver", celestia_getobserver);
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
    RegisterMethod(l, "tojulianday", celestia_tojulianday);
    RegisterMethod(l, "getstarcount", celestia_getstarcount);
    RegisterMethod(l, "getstar", celestia_getstar);
    RegisterMethod(l, "newframe", celestia_newframe);

    lua_pop(l, 1);
}


bool LuaState::init(CelestiaCore* appCore)
{
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

    CreateObjectMetaTable(state);
    CreateObserverMetaTable(state);
    CreateCelestiaMetaTable(state);
    CreatePositionMetaTable(state);

    // Create the celestia object
    lua_pushstring(state, "celestia");
    celestia_new(state, appCore);
    lua_settable(state, LUA_GLOBALSINDEX);

    return true;
}
