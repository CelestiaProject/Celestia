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
#include "CelestiaCoreApplication.h"
#include "imagecapture.h"
#include "url.h"

#include "celx.h"
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

using namespace Eigen;
using namespace std;

#define CLASS(i) ClassNames[(i)]
// names of callback-functions in Lua:



#if LUA_VER >= 0x050100
// Load a Lua library--in Lua 5.1, the luaopen_* functions cannot be called
// directly. They most be invoked through the Lua state.
void openLuaLibrary(lua_State* l,
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
CelestiaCore* getAppCore(lua_State* l, FatalErrors fatalErrors)
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
const char* Celx_SafeGetString(lua_State* l,
                                      int index,
                                      FatalErrors fatalErrors,
                                      const char* errorMsg)
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
lua_Number Celx_SafeGetNumber(lua_State* l, int index, FatalErrors fatalErrors,
                              const char* errorMsg,
                              lua_Number defaultValue)
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
bool Celx_SafeGetBoolean(lua_State* l, int index, FatalErrors fatalErrors,
                              const char* errorMsg,
                              bool defaultValue)
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
void setTable(lua_State* l, const char* field, lua_Number value)
{
    lua_pushstring(l, field);
    lua_pushnumber(l, value);
    lua_settable(l, -3);
}




// ==================== Celscript-object ====================

// create a CelScriptWrapper from a string:
int celscript_from_string(lua_State* l, string& script_text)
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
        return NULL;
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



void loadLuaLibs(lua_State* state);

// ==================== Font Object ====================

int font_new(lua_State* l, TextureFont* f)
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

int texture_new(lua_State* l, Texture* t)
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

void loadLuaLibs(lua_State* state)
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



