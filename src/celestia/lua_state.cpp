
// Moved from celx.cpp by Łukasz Buczyński 27.09.2018

#define SOL_SAFE_USERTYPE 1

#include "lua_state.h" 
#include "celx.h"
#include "CelestiaCoreApplication.h"
#include "celx_celestia.h"

// Maximum timeslice a script may run without
// returning control to celestia
const double MaxTimeslice = 5.0;

// names of callback-functions in Lua:
const char* CleanupCallback = "celestia_cleanup_callback";
const char* KbdCallback = "celestia_keyboard_callback";

const char* EventHandlers = "celestia_event_handlers";
const char* KeyHandler        = "key";
const char* TickHandler       = "tick";
const char* MouseDownHandler  = "mousedown";
const char* MouseUpHandler    = "mouseup";

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
    state = lua.lua_state();
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
        return;
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
        CelestiaCoreApplication* appCore = getAppCore(costate, NoErrors);
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
    lua_getglobal(costate, CleanupCallback);
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
#if LUA_VER >= 0x050200
    status = lua_resume(co, NULL, narg);
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
        CelestiaCoreApplication* appCore = getAppCore(costate, NoErrors);
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
    CelestiaCoreApplication* appCore = getAppCore(costate, NoErrors);
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
    CelestiaCoreApplication* appCore = getAppCore(costate, NoErrors);
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
    if (!costate)
        return true;

    CelestiaCoreApplication* appCore = getAppCore(costate, NoErrors);
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

#if LUA_VER >= 0x050200
    int status = lua_load(state, readStreamChunk, &info, streamname.c_str(),
                          NULL);
#else
    int status = lua_load(state, readStreamChunk, &info, streamname.c_str());
#endif

    if (streamname != "string")
        scriptPath = streamname.c_str();

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
            CelestiaCoreApplication* appCore = getAppCore(co);
            if (appCore != NULL)
            {
                appCore->fatalError(errorMessage);
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
        CelestiaCoreApplication* appCore = getAppCore(costate, NoErrors);
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
        CelestiaCoreApplication* appCore = getAppCore(state, AllErrors);
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

// ==================== Initialization ====================
bool LuaState::init(CelestiaCoreApplication* appCore)
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
    //celestia_new(state, appCore);
    //lua_setglobal(state, "celestia");
    LuaCelestia::registerInLua(lua);
    lua["celestia"] = static_cast<LuaCelestia*>(appCore);
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

