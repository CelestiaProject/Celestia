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

#include <config.h>
#include <cassert>
#include <ctime>
#include <map>
#include <sstream>
#include <utility>
#include <celengine/astro.h>
#include <celengine/asterism.h>
#include <celscript/legacy/cmdparser.h>
#include <celscript/legacy/execution.h>
#include <celengine/timeline.h>
#include <celengine/timelinephase.h>
#include <celutil/debug.h>
#include <celutil/gettext.h>
#include <celestia/celestiacore.h>
#include <celestia/url.h>

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
#include "celx_category.h"


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
    "class_category"
};

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


#if LUA_VERSION_NUM < 503
int lua_isinteger(lua_State *L, int index)
{
    if (lua_type(L, index) == LUA_TNUMBER)
    {
        if (lua_tonumber(L, index) == lua_tointeger(L, index))
            return 1;
    }
    return 0;
}
#endif


static void openLuaLibrary(lua_State* l,
                           const char* name,
                           lua_CFunction func)
{
#if LUA_VERSION_NUM >= 502
    luaL_requiref(l, name, func, 1);
#else
    lua_pushcfunction(l, func);
    lua_pushstring(l, name);
    lua_call(l, 1, 0);
#endif
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
    state = luaL_newstate();
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
#if LUA_VERSION_NUM >= 504
    int nresults;
    status = lua_resume(co, nullptr, narg, &nresults);
#elif LUA_VERSION_NUM >= 502
    status = lua_resume(co, nullptr, narg);
#else
    status = lua_resume(co, narg);
#endif
    if (status == 0 || status == LUA_YIELD)
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
            openLuaLibrary(costate, LUA_LOADLIBNAME, luaopen_package);
            openLuaLibrary(costate, LUA_IOLIBNAME, luaopen_io);
            openLuaLibrary(costate, LUA_OSLIBNAME, luaopen_os);
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

    return result;
}


// Returns true if a handler is registered for the key
bool LuaState::handleKeyEvent(const char* key)
{
    CelestiaCore* appCore = getAppCore(costate, NoErrors);
    if (appCore == nullptr)
        return false;

    // get the registered event table
    lua_getfield(costate, LUA_REGISTRYINDEX, EventHandlers);
    if (!lua_istable(costate, -1))
    {
        cerr << "Missing event handler table";
        lua_pop(costate, 1);
        return false;
    }

    bool handled = false;
    lua_getfield(costate, -1, KeyHandler);
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
    lua_getfield(costate, LUA_REGISTRYINDEX, EventHandlers);
    if (!lua_istable(costate, -1))
    {
        cerr << "Missing event handler table";
        lua_pop(costate, 1);
        return false;
    }

    bool handled = false;
    lua_getfield(costate, -1, down ? MouseDownHandler : MouseUpHandler);
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
    lua_getfield(costate, LUA_REGISTRYINDEX, EventHandlers);
    if (!lua_istable(costate, -1))
    {
        cerr << "Missing event handler table";
        lua_pop(costate, 1);
        return false;
    }

    bool handled = false;
    lua_getfield(costate, -1, TickHandler);
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


int LuaState::loadScript(istream& in, const fs::path& streamname)
{
    char buf[4096];
    ReadChunkInfo info;
    info.buf = buf;
    info.bufSize = sizeof(buf);
    info.in = &in;

    if (streamname != "string")
    {
        lua_pushstring(state, "celestia-scriptpath");
        lua_pushstring(state, streamname.string().c_str());
        lua_settable(state, LUA_REGISTRYINDEX);
    }

#if LUA_VERSION_NUM >= 502
    int status = lua_load(state, readStreamChunk, &info,
                          streamname.string().c_str(), nullptr);
#else
    int status = lua_load(state, readStreamChunk, &info,
                          streamname.string().c_str());
#endif
    if (status != 0)
        cout << "Error loading script: " << lua_tostring(state, -1) << '\n';

    return status;
}

int LuaState::loadScript(const std::string& s)
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

        cout << "Error: " << errorMessage << '\n';
        CelestiaCore* appCore = getAppCore(co);
        if (appCore != nullptr)
            appCore->fatalError(errorMessage);

        return 1; // just the error string
    }

    if (ioMode == Asking)
    {
        // timeout now is used to first only display warning, and 1s
        // later allow response to avoid accidental activation
        timeout = getTime() + 1.0;
    }

    // The thread status is zero if it has terminated normally
    if (lua_status(co) == 0)
        alive = false;

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
            openLuaLibrary(costate, LUA_LOADLIBNAME, luaopen_package);
            openLuaLibrary(costate, LUA_IOLIBNAME, luaopen_io);
            openLuaLibrary(costate, LUA_OSLIBNAME, luaopen_os);
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
    // Import the base, table, string, and math libraries
    openLuaLibrary(state, "", luaopen_base);
    openLuaLibrary(state, LUA_MATHLIBNAME, luaopen_math);
    openLuaLibrary(state, LUA_TABLIBNAME, luaopen_table);
    openLuaLibrary(state, LUA_STRLIBNAME, luaopen_string);
#if LUA_VERSION_NUM >= 502
    openLuaLibrary(state, LUA_COLIBNAME, luaopen_coroutine);
#endif
    // Make the package library, except the loadlib function, available
    // for celx regardless of script system access policy.
    allowLuaPackageAccess();

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
    lua_getglobal(state, "package");
    lua_pushstring(state, s.c_str());
    lua_setfield(state, -2, "path");
    lua_pop(state, 1);
}

// ==================== Load Libraries ================================================

static void loadLuaLibs(lua_State* state)
{
    openLuaLibrary(state, LUA_DBLIBNAME, luaopen_debug);

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
    CreateCategoryMetaTable(state);
    ExtendCelestiaMetaTable(state);
    ExtendObjectMetaTable(state);

#ifndef GL_ES
    LoadLuaGraphicsLibrary(state);
#endif
}


void LuaState::allowSystemAccess()
{
    openLuaLibrary(state, LUA_LOADLIBNAME, luaopen_package);
    openLuaLibrary(state, LUA_IOLIBNAME, luaopen_io);
    openLuaLibrary(state, LUA_OSLIBNAME, luaopen_os);
    ioMode = IOAllowed;
}


// Permit access to the package library, but prohibit use of the loadlib
// function.
void LuaState::allowLuaPackageAccess()
{
    openLuaLibrary(state, LUA_LOADLIBNAME, luaopen_package);

    // Disallow loadlib
    lua_getglobal(state, "package");
    lua_pushnil(state);
    lua_setfield(state, -2, "loadlib");
    lua_pop(state, 1);
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

bool CelxLua::isValid(int i) const
{
    int argc = lua_gettop(m_lua);
    return i > 0 && i <= argc;
}

bool CelxLua::safeIsValid(int i, FatalErrors errors, const char *msg)
{
    if (!isValid(i))
    {
        if (errors & WrongArgc)
            doError(msg);
        return false;
    }
    return true;
}

CelxLua::CelxLua(lua_State* l) :
m_lua(l)
{
}

bool CelxLua::isType(int index, int type) const
{
    return Celx_istype(m_lua, index, type);
}

Value *CelxLua::getValue(int index)
{
    Value *v = nullptr;
    if (isInteger(index))
        v = new Value((double)getInt(index));
    else if (isNumber(index))
        v = new Value(getNumber(index));
    else if (isBoolean(index))
        v = new Value(getBoolean(index));
    else if (isString(index))
        v = new Value(getString(index));
    else if (isTable(index))
    {
        ::Array *array = new ::Array;
        Hash *hash = new Hash;
        push();
        while(lua_next(m_lua, index) != 0)
        {
            if (isInteger(-2))
            {
                if (hash != nullptr)
                {
                    delete hash;
                    hash = nullptr;
                }
                if (array != nullptr)
                {
                    array->push_back(getValue(-1));
                }
            }
            else if (isString(-2))
            {
                if (array != nullptr)
                {
                    delete array;
                    array = nullptr;
                }
                if (hash != nullptr)
                {
                    hash->addValue(getString(-2), *getValue(-1));
                }
            }
            pop(1);
            if (array == nullptr && hash == nullptr)
                break;
        }
        pop(1);
        if (hash != nullptr)
            v = new Value(hash);
        else if (array != nullptr)
            v = new Value(array);
    }
    return v;
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

const char *CelxLua::safeGetNonEmptyString(int index,
                                        FatalErrors fatalErrors,
                                        const char *errorMessage)
{
    const char *s = safeGetString(index, fatalErrors, errorMessage);
    if (s == nullptr || *s == '\0')
    {
        doError(errorMessage);
        return nullptr;
    }
    return s;
}

bool CelxLua::safeGetBoolean(int index,
                             FatalErrors fatalErrors,
                             const char* errorMessage,
                             bool defaultValue)
{
    return Celx_SafeGetBoolean(m_lua, index, fatalErrors, errorMessage, defaultValue);
}


void CelxLua::newVector(const Vector3d& v)
{
    vector_new(m_lua, v);
}


void CelxLua::newPosition(const UniversalCoord& uc)
{
    position_new(m_lua, uc);
}


void CelxLua::newRotation(const Quaterniond& q)
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

void CelxLua::newPhase(const shared_ptr<const TimelinePhase>& phase)
{
    phase_new(m_lua, phase);
}

Vector3d* CelxLua::toVector(int n)
{
    return to_vector(m_lua, n);
}

Quaterniond* CelxLua::toRotation(int n)
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
