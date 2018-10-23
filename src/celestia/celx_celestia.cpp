// celx_celestia.cpp
//
// Copyright (C) 2003-2008, the Celestia Development Team
//
// Lua script extensions for Celestia: Celestia object
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include "lua_state.h"
#include "celx.h"
#include "celx_celestia.h"
#include "CelestiaCoreApplication.h"
#include "celx_object.h"
#include "celx_vector.h"
#include "celx_rotation.h"
#include "celx_position.h"
#include "celx_frame.h"
#include "celx_phase.h"
#include "celx_observer.h"
#include "celx_gl.h"


// Moved from celx.cpp by Łukasz Buczyński 27.09.2018

// ==================== Celestia-object ====================
int celestia_new(lua_State* l, CelestiaCoreApplication* appCore)
{
    CelestiaCoreApplication** ud = reinterpret_cast<CelestiaCoreApplication**>(lua_newuserdata(l, sizeof(CelestiaCoreApplication*)));
    *ud = appCore;

    Celx_SetClass(l, Celx_Celestia);

    return 1;
}

static LuaCelestia* to_celestia(lua_State* l, int index)
{
    /*CelestiaCoreApplication** appCore = static_cast<CelestiaCoreApplication**>(Celx_CheckUserData(l, index, Celx_Celestia));
    CelestiaCoreApplication** appCore = static_cast<CelestiaCoreApplication**>(lua_touserdata(l, index));
    if (appCore == NULL)
        return NULL;
    else
        return *appCore;*/
    sol::object o(l, index);
    if (o.is<LuaCelestia*>())
        return o.as<LuaCelestia*>();
    else return NULL;
}

static CelestiaCoreApplication* this_celestia(lua_State* l)
{
    CelestiaCoreApplication* appCore = to_celestia(l, 1);
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
                for (AsterismList::const_iterator iter = asterisms->begin();
                     iter != asterisms->end(); iter++)
                {
                    Asterism* ast = *iter;
                    if (compareIgnoringCase(constellation, ast->getName(false)) == 0)
                        ast->setOverrideColor(constellationColor);
                }
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

// -----------------------------------------------------------------------------
// Star Color

static int celestia_getstarcolor(lua_State* l)
{
    Celx_CheckArgs(l, 1, 1, "No argument expected in celestia:getstarcolor");
    CelestiaCore* appCore = this_celestia(l);

    Renderer* renderer = appCore->getRenderer();
    if (renderer == NULL)
    {
        Celx_DoError(l, "Internal Error: renderer is NULL!");
    }
    else
    {
        const ColorTemperatureTable* starColor = renderer->getStarColorTable();

            if (starColor == GetStarColorTable(ColorTable_Enhanced))
            {
                lua_pushstring(l, "enhanced");
            }
            else if (starColor == GetStarColorTable(ColorTable_Blackbody_D65))
            {
                lua_pushstring(l, "blackbody_d65");
            }
            else
            {
                lua_pushstring(l, "invalid starcolor");
            }
     }
    return 1;
}

static int celestia_setstarcolor(lua_State* l)
{
    Celx_CheckArgs(l, 2, 2, "One argument expected in celestia:setstarcolor");
    CelestiaCore* appCore = this_celestia(l);

    string starColor = Celx_SafeGetString(l, 2, AllErrors, "Argument to celestia:setstarcolor must be a string");
    Renderer* renderer = appCore->getRenderer();
    if (renderer == NULL)
    {
        Celx_DoError(l, "Internal Error: renderer is NULL!");
    }
    else
    {
        if (starColor == "blackbody_d65")
        {
            renderer->setStarColorTable(GetStarColorTable(ColorTable_Blackbody_D65));
        }
        else if (starColor == "enhanced")
        {
            renderer->setStarColorTable(GetStarColorTable(ColorTable_Enhanced));
        }
        else
        {
            Celx_DoError(l, "Invalid starcolor");
        }
        appCore->notifyWatchers(CelestiaCore::RenderFlagsChanged);

    }
    return 0;
}

// -----------------------------------------------------------------------------

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
            return 0;
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
            return 0;
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
            return 0;
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
        lua_getglobal(l, KbdCallback);
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
    Celx_CheckArgs(l, 1, 1, "No argument expected for celestia:getscriptpath()");
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


//Audio support by Victor, modified by Vincent & Alexell & Pirogronian
static int celestia_play(lua_State* l)
{
    Celx_CheckArgs(l, 3, 7, "Two to Five arguments expected to function celestia:play");
    CelestiaCoreApplication* appCore = this_celestia(l);
    int channel = (int)Celx_SafeGetNumber(l, 2, AllErrors, "First argument to celestia:play must be a number (channel)");
    float volume = (float)Celx_SafeGetNumber(l, 3, AllErrors, "Second argument to celestia:play must be a number (volume)");
    bool loop = (bool)Celx_SafeGetBoolean(l, 4, WrongType, "Third argument to celestia:play must be a boolean (loop)");
    bool nopause = (bool)Celx_SafeGetBoolean(l, 5, WrongType, "Fourth argument to celestia:play must be false or true (nopause)");
    const char* filename = Celx_SafeGetString(l, 6, WrongType, "Fifth argument to celestia:play must be a string (filename)");

    if(!filename)
        appCore->playSoundFile(channel,volume,loop, "-", nopause);
    else
        appCore->playSoundFile(channel,volume,loop,filename, nopause);

    return 0;
}

//SCRIPT IMAGE START: Author Vincent
static int celestia_overlay(lua_State* l)
{
    Celx_CheckArgs(l, 2, 7, "One to Six arguments expected to function celestia:overlay");

    CelestiaCore* appCore = this_celestia(l);
    double duration = Celx_SafeGetNumber(l, 2, WrongType, "First argument to celestia:overlay must be a number (duration)", 3.0);
    float xoffset = Celx_SafeGetNumber(l, 3, WrongType, "Second argument to celestia:overlay must be a number (xoffset)", 0.0);
    float yoffset = Celx_SafeGetNumber(l, 4, WrongType, "Third argument to celestia:overlay must be a number (yoffset)", 0.0);
    float alpha = Celx_SafeGetNumber(l, 5, WrongType, "Fourth argument to celestia:overlay must be a number (alpha)", 1.0);
    const char* filename = Celx_SafeGetString(l, 6, AllErrors, "Fifth argument to celestia:overlay must be a string (filename)");
    int fitscreen = Celx_SafeGetNumber(l, 7, WrongType, "Sixth argument to celestia:overlay must be a number (fitscreen)", 0);

    appCore->setScriptImage(duration, xoffset, yoffset, alpha, filename, fitscreen);

    return 0;
}

static int celestia_verbosity(lua_State* l)
{
    Celx_CheckArgs(l, 2, 2, "One argument expected to function celestia:verbosity");

    CelestiaCore* appCore = this_celestia(l);
    int level = Celx_SafeGetNumber(l, 2, WrongType, "First argument to celestia:verbosity must be a number (level)", 2.0);

        appCore->setHudDetail(level);

    return 0;
}
//SCRIPT IMAGE END


void CreateCelestiaMetaTable(lua_State* l)
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

    // New CELX command for Star Color
    Celx_RegisterMethod(l, "getstarcolor", celestia_getstarcolor);
    Celx_RegisterMethod(l, "setstarcolor", celestia_setstarcolor);

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
    Celx_RegisterMethod(l, "play", celestia_play);
    Celx_RegisterMethod(l, "overlay", celestia_overlay);
    Celx_RegisterMethod(l, "verbosity", celestia_verbosity);


    lua_pop(l, 1);
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

void ExtendCelestiaMetaTable(lua_State* l)
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


void LuaCelestia::registerInLua(sol::state &l)
{
    LuaObject::registerInLua(l);
    
    auto type = l.create_simple_usertype<LuaCelestia>();
    type.set("__tostring", &celestia_tostring);
    type.set("flash", &celestia_flash);
    type.set("print", &celestia_print);
    type.set("gettextwidth", celestia_gettextwidth);
    type.set("show", &celestia_show);
    type.set("setaltazimuthmode", &celestia_setaltazimuthmode);
    type.set("getaltazimuthmode", &celestia_getaltazimuthmode);
    type.set("hide", &celestia_hide);
    type.set("getrenderflags", &celestia_getrenderflags);
    type.set("setrenderflags", &celestia_setrenderflags);
    type.set("getscreendimension", &celestia_getscreendimension);
    type.set("showlabel", &celestia_showlabel);
    type.set("hidelabel", &celestia_hidelabel);
    type.set("getlabelflags", &celestia_getlabelflags);
    type.set("setlabelflags", &celestia_setlabelflags);
    type.set("getorbitflags", &celestia_getorbitflags);
    type.set("setorbitflags", &celestia_setorbitflags);
    type.set("showconstellations", &celestia_showconstellations);
    type.set("hideconstellations", &celestia_hideconstellations);
    type.set("setconstellationcolor", &celestia_setconstellationcolor);
    type.set("setlabelcolor", &celestia_setlabelcolor);
    type.set("getlabelcolor", &celestia_getlabelcolor);
    type.set("setlinecolor",  &celestia_setlinecolor);
    type.set("getlinecolor",  &celestia_getlinecolor);
    type.set("settextcolor",  &celestia_settextcolor);
    type.set("gettextcolor",  &celestia_gettextcolor);
    type.set("getoverlayelements", &celestia_getoverlayelements);
    type.set("setoverlayelements", &celestia_setoverlayelements);
    type.set("getfaintestvisible", &celestia_getfaintestvisible);
    type.set("setfaintestvisible", &celestia_setfaintestvisible);
    type.set("getgalaxylightgain", &celestia_getgalaxylightgain);
    type.set("setgalaxylightgain", &celestia_setgalaxylightgain);
    type.set("setminfeaturesize", &celestia_setminfeaturesize);
    type.set("getminfeaturesize", &celestia_getminfeaturesize);
    type.set("getobserver", &celestia_getobserver);
    type.set("getobservers", &celestia_getobservers);
    type.set("getselection", &celestia_getselection);
    type.set("find", &celestia_find);
    type.set("select", &celestia_select);
    type.set("mark", &celestia_mark);
    type.set("unmark", &celestia_unmark);
    type.set("unmarkall", &celestia_unmarkall);
    type.set("gettime", &celestia_gettime);
    type.set("settime", &celestia_settime);
    type.set("ispaused", &celestia_ispaused);
    type.set("synchronizetime", &celestia_synchronizetime);
    type.set("istimesynchronized", &celestia_istimesynchronized);
    type.set("gettimescale", &celestia_gettimescale);
    type.set("settimescale", &celestia_settimescale);
    type.set("getambient", &celestia_getambient);
    type.set("setambient", &celestia_setambient);
    type.set("getminorbitsize", &celestia_getminorbitsize);
    type.set("setminorbitsize", &celestia_setminorbitsize);
    type.set("getstardistancelimit", &celestia_getstardistancelimit);
    type.set("setstardistancelimit", &celestia_setstardistancelimit);
    type.set("getstarstyle", &celestia_getstarstyle);
    type.set("setstarstyle", &celestia_setstarstyle);
    type.set("getstarcolor", &celestia_getstarcolor);
    type.set("setstarcolor", &celestia_setstarcolor);
    type.set("gettextureresolution", &celestia_gettextureresolution);
    type.set("settextureresolution", &celestia_settextureresolution);
    type.set("tojulianday", &celestia_tojulianday);
    type.set("fromjulianday", &celestia_fromjulianday);
    type.set("utctotdb", &celestia_utctotdb);
    type.set("tdbtoutc", &celestia_tdbtoutc);
    type.set("getsystemtime", &celestia_getsystemtime);
    type.set("getstarcount", &celestia_getstarcount);
    type.set("getdsocount", &celestia_getdsocount);
    type.set("getstar", &celestia_getstar);
    type.set("getdso", &celestia_getdso);
    type.set("newframe", &celestia_newframe);
    type.set("newvector", &celestia_newvector);
    type.set("newposition", &celestia_newposition);
    type.set("newrotation", &celestia_newrotation);
    type.set("getscripttime", &celestia_getscripttime);
    type.set("requestkeyboard", &celestia_requestkeyboard);
    type.set("createcelscript", celestia_createcelscript);
    type.set("requestsystemaccess", &celestia_requestsystemaccess);
    type.set("getscriptpath", &celestia_getscriptpath);
    type.set("runscript", &celestia_runscript);
    type.set("registereventhandler", &celestia_registereventhandler);
    type.set("geteventhandler", &celestia_geteventhandler);
    type.set("stars", &celestia_stars);
    type.set("dsos", &celestia_dsos);
    type.set("windowbordersvisible", &celestia_windowbordersvisible);
    type.set("setwindowbordersvisible", &celestia_setwindowbordersvisible);
    type.set("seturl", &celestia_seturl);
    type.set("geturl", &celestia_geturl);
    type.set("play", &celestia_play);
    type.set("log", &celestia_log);
    type.set("settimeslice", &celestia_settimeslice);
    type.set("setluahook", &celestia_setluahook);
    type.set("getparamstring", &celestia_getparamstring);
    type.set("getfont", &celestia_getfont);
    type.set("gettitlefont", &celestia_gettitlefont);
    type.set("loadtexture", &celestia_loadtexture);
    type.set("loadfont", &celestia_loadfont);
    l.set_usertype("Celestia", type);
}
