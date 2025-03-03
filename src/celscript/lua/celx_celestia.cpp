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

#include <algorithm>
#include <cstdint>
#include <iostream>
#include <string_view>

#include <fmt/format.h>

#include <celcompat/filesystem.h>
#include <celengine/category.h>
#include <celengine/texture.h>
#include <celestia/audiosession.h>
#include <celestia/configfile.h>
#include <celestia/hud.h>
#include <celestia/url.h>
#include <celestia/celestiacore.h>
#include <celestia/view.h>
#include <celscript/common/scriptmaps.h>
#include <celttf/truetypefont.h>
#include <celutil/gettext.h>
#include <celutil/logger.h>
#include <celutil/stringutils.h>
#include "celx.h"
#include "celx_internal.h"
#include "celx_celestia.h"
#include "celx_frame.h"
#include "celx_misc.h"
#include "celx_observer.h"
#include "celx_object.h"
#include "celx_position.h"
#include "celx_rotation.h"
#include "celx_vector.h"
#include "celx_category.h"

using namespace std;
using namespace std::string_view_literals;
using namespace Eigen;
using namespace celestia;
using namespace celestia::scripts;
using celestia::util::GetLogger;

extern const char* KbdCallback;
extern const char* CleanupCallback;

extern const char* EventHandlers;

extern const char* KeyHandler;
extern const char* TickHandler;
extern const char* MouseDownHandler;
extern const char* MouseUpHandler;

LuaState *getLuaStateObject(lua_State*);

void PushClass(lua_State*, int);
void setTable(lua_State*, const char*, lua_Number);
ObserverFrame::CoordinateSystem parseCoordSys(std::string_view);

static fs::path GetScriptPath(lua_State* l)
{
    lua_Debug ar;
    lua_getstack(l, 1, &ar);
    lua_getinfo(l, "S", &ar);
    auto* base_dir = ar.source; // Lua file from which we are called
    if (base_dir[0] == '@')
        base_dir++;
    return fs::path(base_dir).parent_path();
}

// ==================== Celestia-object ====================
int celestia_new(lua_State* l, CelestiaCore* appCore)
{
    CelestiaCore** ud = static_cast<CelestiaCore**>(lua_newuserdata(l, sizeof(CelestiaCore*)));
    *ud = appCore;

    Celx_SetClass(l, Celx_Celestia);

    return 1;
}

CelestiaCore* to_celestia(lua_State* l, int index)
{
    auto** appCore = static_cast<CelestiaCore**>(Celx_CheckUserData(l, index, Celx_Celestia));
    return appCore ? *appCore : nullptr;
}

CelestiaCore* this_celestia(lua_State* l)
{
    CelestiaCore* appCore = to_celestia(l, 1);
    if (appCore == nullptr)
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
        duration = 1.5;

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
        duration = 1.5;

    appCore->showText(s, horig, vorig, hoff, voff, duration);

    return 0;
}

static int celestia_printatpixel(lua_State* l)
{
    Celx_CheckArgs(l, 2, 5, "One to four arguments expected to function celestia:printatpixel");

    CelestiaCore* appCore = this_celestia(l);
    const char* s = Celx_SafeGetString(l, 2, AllErrors, "First argument to celestia:print must be a string");
    double duration = Celx_SafeGetNumber(l, 3, WrongType, "Second argument to celestia:print must be a number", 1.5);
    int x = (int)Celx_SafeGetNumber(l, 4, WrongType, "Third argument to celestia:print must be a number", 0.0);
    int y = (int)Celx_SafeGetNumber(l, 5, WrongType, "Fourth argument to celestia:print must be a number", 0.0);

    if (duration < 0.0)
        duration = 1.5;

    appCore->showTextAtPixel(s, x, y, duration);

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

static int celestia_getscreendpi(lua_State* l)
{
    Celx_CheckArgs(l, 1, 1, "No arguments expected for celestia:getscreendp()");

    CelestiaCore* appCore = this_celestia(l);
    lua_pushnumber(l, appCore->getScreenDpi());

    return 1;
}

static int celestia_setscreendpi(lua_State* l)
{
    Celx_CheckArgs(l, 2, 2, "One argument expected for celestia:setscreendpi()");
    int screenDpi = (int)Celx_SafeGetNumber(l, 2, AllErrors, "Argument to celestia:setscreendpi() must be a number");
    screenDpi = max(screenDpi, 1);

    CelestiaCore* appCore = this_celestia(l);
    appCore->setScreenDpi(screenDpi);

    return 0;
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
    if (!lua_isboolean(l, -1))
    {
        Celx_DoError(l, "Argument for celestia:setaltazimuthmode must be a boolean");
        return 0;
    }

    enable = lua_toboolean(l, -1) != 0;
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
    RenderFlags flags = RenderFlags::ShowNothing;
    const auto &RenderFlagMap = appCore->scriptMaps().RenderFlagMap;
    for (int i = 2; i <= argc; i++)
    {
        std::string_view renderFlag = Celx_SafeGetString(l, i, AllErrors, "Arguments to celestia:show() must be strings");
        if (renderFlag == "lightdelay"sv)
            appCore->setLightDelayActive(true);
        else if (auto it = RenderFlagMap.find(renderFlag); it != RenderFlagMap.end())
            flags |= it->second;
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
    RenderFlags flags = RenderFlags::ShowNothing;
    const auto &RenderFlagMap = appCore->scriptMaps().RenderFlagMap;
    for (int i = 2; i <= argc; i++)
    {
        std::string_view renderFlag = Celx_SafeGetString(l, i, AllErrors, "Arguments to celestia:hide() must be strings");
        if (renderFlag == "lightdelay"sv)
            appCore->setLightDelayActive(false);
        else if (auto it = RenderFlagMap.find(renderFlag); it != RenderFlagMap.end())
            flags |= it->second;
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
        return 0;
    }

    RenderFlags renderFlags = appCore->getRenderer()->getRenderFlags();
    lua_pushnil(l);
    const auto& renderFlagMap = appCore->scriptMaps().RenderFlagMap;
    while (lua_next(l, -2) != 0)
    {
        std::string_view key;
        if (lua_isstring(l, -2))
        {
            key = lua_tostring(l, -2);
        }
        else
        {
            Celx_DoError(l, "Keys in table-argument to celestia:setrenderflags() must be strings");
            return 0;
        }

        bool value = false;
        if (lua_isboolean(l, -1))
        {
            value = lua_toboolean(l, -1) != 0;
        }
        else
        {
            Celx_DoError(l, "Values in table-argument to celestia:setrenderflags() must be boolean");
            return 0;
        }

        if (key == "lightdelay"sv)
        {
            appCore->setLightDelayActive(value);
        }
        else if (auto it = renderFlagMap.find(key); it != renderFlagMap.end())
        {
            if (value)
                renderFlags |= it->second;
            else
                renderFlags &= ~(it->second);
        }
        else
        {
            GetLogger()->warn("Unknown key: {}\n", key);
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
    const RenderFlags renderFlags = appCore->getRenderer()->getRenderFlags();
    for (const auto& rfm : appCore->scriptMaps().RenderFlagMap)
    {
        lua_pushlstring(l, rfm.first.data(), rfm.first.size());
        lua_pushboolean(l, util::is_set(renderFlags, rfm.second));
        lua_settable(l,-3);
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
    int w, h;
    CelestiaCore* appCore = to_celestia(l, 1);
    appCore->getRenderer()->getViewport(nullptr, nullptr, &w, &h);
    lua_pushnumber(l, w);
    lua_pushnumber(l, h);
    return 2;
}

int celestia_getwindowdimension(lua_State* l)
{
    Celx_CheckArgs(l, 1, 1, "No arguments expected for celestia:getwindowdimension()");
    // error checking only:
    this_celestia(l);
    // Get the dimensions of the current viewport
    CelestiaCore* appCore = to_celestia(l, 1);
    auto dimension = appCore->getWindowDimension();
    lua_pushnumber(l, get<0>(dimension));
    lua_pushnumber(l, get<1>(dimension));
    return 2;
}

int celestia_getsafeareainsets(lua_State* l)
{
    Celx_CheckArgs(l, 1, 1, "No arguments expected for celestia:getsafeareainsets()");
    // error checking only:
    this_celestia(l);
    CelestiaCore* appCore = to_celestia(l, 1);
    const auto &edgeInsets = appCore->getSafeAreaInsets();
    lua_pushnumber(l, get<0>(edgeInsets));
    lua_pushnumber(l, get<1>(edgeInsets));
    lua_pushnumber(l, get<2>(edgeInsets));
    lua_pushnumber(l, get<3>(edgeInsets));
    return 4;
}

int celestia_setsafeareainsets(lua_State* l)
{
    Celx_CheckArgs(l, 5, 5, "Four arguments expected for celestia:setsafeareainsets()");

    CelestiaCore* appCore = getAppCore(l, AllErrors);

    int left = (int)Celx_SafeGetNumber(l, 2, WrongType, "First argument to celestia:setsafeareainsets() must be a number", 0.0);
    int top = (int)Celx_SafeGetNumber(l, 3, WrongType, "Second argument to celestia:setsafeareainsets() must be a number", 0.0);
    int right = (int)Celx_SafeGetNumber(l, 4, WrongType, "Third argument to celestia:setsafeareainsets() must be a number", 0.0);
    int bottom = (int)Celx_SafeGetNumber(l, 5, WrongType, "Fourth argument to celestia:setsafeareainsets() must be a number", 0.0);
    appCore->setSafeAreaInsets(left, top, right, bottom);
    return 0;
}

static int celestia_getlayoutdirection(lua_State* l)
{
    Celx_CheckArgs(l, 1, 1, "No argument expected in celestia:getlayoutdirection");
    switch (this_celestia(l)->getLayoutDirection())
    {
    case celestia::LayoutDirection::LeftToRight:
        lua_pushstring(l, "ltr");
        break;
    case celestia::LayoutDirection::RightToLeft:
        lua_pushstring(l, "rtl");
        break;
    default:
        lua_pushstring(l, "invalid layoutDirection");
        break;
    }
    return 1;
}

static int celestia_setlayoutdirection(lua_State* l)
{
    Celx_CheckArgs(l, 2, 2, "One argument expected in celestia:setlayoutdirection");
    auto appCore = this_celestia(l);

    string layoutDirection = Celx_SafeGetString(l, 2, AllErrors, "Argument to celestia:setlayoutdirection must be a string");
    if (layoutDirection == "ltr") // NOSONAR
        appCore->setLayoutDirection(LayoutDirection::LeftToRight);
    else if (layoutDirection == "rtl")
        appCore->setLayoutDirection(LayoutDirection::RightToLeft);
    else
       Celx_DoError(l, "Invalid layoutDirection");
    return 0;
}

static int celestia_showlabel(lua_State* l)
{
    Celx_CheckArgs(l, 1, 1000, "Bad method call!");
    CelestiaCore* appCore = this_celestia(l);

    int argc = lua_gettop(l);
    RenderLabels flags = RenderLabels::NoLabels;
    const auto &LabelFlagMap = appCore->scriptMaps().LabelFlagMap;
    for (int i = 2; i <= argc; i++)
    {
        std::string_view labelFlag = Celx_SafeGetString(l, i, AllErrors, "Arguments to celestia:showlabel() must be strings");
        if (auto it = LabelFlagMap.find(labelFlag); it != LabelFlagMap.end())
            flags |= it->second;
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
    RenderLabels flags = RenderLabels::NoLabels;
    const auto &LabelFlagMap = appCore->scriptMaps().LabelFlagMap;
    for (int i = 2; i <= argc; i++)
    {
        std::string_view labelFlag = Celx_SafeGetString(l, i, AllErrors, "Arguments to celestia:hidelabel() must be strings");
        if (auto it = LabelFlagMap.find(labelFlag); it != LabelFlagMap.end())
            flags |= it->second;
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
        return 0;
    }

    RenderLabels labelFlags = appCore->getRenderer()->getLabelMode();
    const auto &LabelFlagMap = appCore->scriptMaps().LabelFlagMap;
    lua_pushnil(l);
    while (lua_next(l, -2) != 0)
    {
        std::string_view key;
        if (lua_isstring(l, -2))
        {
            key = lua_tostring(l, -2);
        }
        else
        {
            Celx_DoError(l, "Keys in table-argument to celestia:setlabelflags() must be strings");
            return 0;
        }

        bool value = false;
        if (lua_isboolean(l, -1))
        {
            value = lua_toboolean(l, -1) != 0;
        }
        else
        {
            Celx_DoError(l, "Values in table-argument to celestia:setlabelflags() must be boolean");
            return 0;
        }

        if (auto it = LabelFlagMap.find(key); it == LabelFlagMap.end())
        {
            GetLogger()->warn("Unknown key: {}\n", key);
        }
        else
        {
            RenderLabels flag = it->second;
            if (value)
                labelFlags |= flag;
            else
                labelFlags &= ~flag;
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
    const RenderLabels labelFlags = appCore->getRenderer()->getLabelMode();
    for (const auto& lfm : appCore->scriptMaps().LabelFlagMap)
    {
        lua_pushlstring(l, lfm.first.data(), lfm.first.size());
        lua_pushboolean(l, util::is_set(labelFlags, lfm.second));
        lua_settable(l,-3);
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
        return 0;
    }

    BodyClassification orbitFlags = appCore->getRenderer()->getOrbitMask();
    lua_pushnil(l);
    const auto &BodyTypeMap = appCore->scriptMaps().BodyTypeMap;
    while (lua_next(l, -2) != 0)
    {
        std::string_view key;
        if (lua_isstring(l, -2))
        {
            key = lua_tostring(l, -2);
        }
        else
        {
            Celx_DoError(l, "Keys in table-argument to celestia:setorbitflags() must be strings");
            return 0;
        }

        bool value = false;
        if (lua_isboolean(l, -1))
        {
            value = lua_toboolean(l, -1) != 0;
        }
        else
        {
            Celx_DoError(l, "Values in table-argument to celestia:setorbitflags() must be boolean");
            return 0;
        }

        if (auto it = BodyTypeMap.find(key); it == BodyTypeMap.end())
        {
            GetLogger()->warn("Unknown key: {}\n", key);
        }
        else
        {
            BodyClassification flag = it->second;
            if (value)
                orbitFlags |= flag;
            else
                orbitFlags &= ~flag;
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
    const BodyClassification orbitFlags = appCore->getRenderer()->getOrbitMask();
    for (const auto& btm : appCore->scriptMaps().BodyTypeMap)
    {
        lua_pushlstring(l, btm.first.data(), btm.first.size());
        lua_pushboolean(l, util::is_set(orbitFlags, btm.second));
        lua_settable(l,-3);
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
        for (auto& ast : *asterisms)
            ast.setActive(true);
        return 0;
    }

    if (!lua_istable(l, 2))
    {
        Celx_DoError(l, "Argument to celestia:showconstellations() must be a table");
        return 0;
    }

    lua_pushnil(l);
    while (lua_next(l, -2) != 0)
    {
        std::string_view constellation;
        if (lua_isstring(l, -1))
        {
            constellation = lua_tostring(l, -1);
        }
        else
        {
            Celx_DoError(l, "Values in table-argument to celestia:showconstellations() must be strings");
            return 0;
        }

        if (auto it = std::find_if(asterisms->begin(), asterisms->end(),
                                    [&constellation](const auto& ast)
                                    {
                                        return compareIgnoringCase(constellation, ast.getName(false)) == 0;
                                    });
            it != asterisms->end())
        {
            it->setActive(true);
        }

        lua_pop(l,1);
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
        for (auto& ast : *asterisms)
            ast.setActive(false);
        return 0;
    }

    if (!lua_istable(l, 2))
    {
        Celx_DoError(l, "Argument to celestia:hideconstellations() must be a table");
        return 0;
    }

    lua_pushnil(l);
    while (lua_next(l, -2) != 0)
    {
        std::string_view constellation;
        if (lua_isstring(l, -1))
        {
            constellation = lua_tostring(l, -1);
        }
        else
        {
            Celx_DoError(l, "Values in table-argument to celestia:hideconstellations() must be strings");
            return 0;
        }

        if (auto it = std::find_if(asterisms->begin(), asterisms->end(),
                                   [&constellation](const auto& ast)
                                   {
                                       return compareIgnoringCase(constellation, ast.getName(false)) == 0;
                                   });
            it != asterisms->end())
        {
            it->setActive(false);
        }

        lua_pop(l,1);
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
        for (auto& ast : *asterisms)
            ast.setOverrideColor(constellationColor);
        return 0;
    }

    if (!lua_istable(l, 5))
    {
        Celx_DoError(l, "Fourth argument to celestia:setconstellationcolor() must be a table");
        return 0;
    }

    lua_pushnil(l);
    while (lua_next(l, -2) != 0)
    {
        if (!lua_isstring(l, -1))
        {
            Celx_DoError(l, "Values in table-argument to celestia:setconstellationcolor() must be strings");
            return 0;
        }

        std::string_view constellation = lua_tostring(l, -1);
        if (auto it = std::find_if(asterisms->begin(), asterisms->end(),
                                    [&constellation](const auto& ast)
                                    {
                                        return compareIgnoringCase(constellation, ast.getName(false)) == 0;
                                    });
            it != asterisms->end())
        {
            it->setOverrideColor(constellationColor);
        }

        lua_pop(l,1);
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
        return 0;
    }

    auto overlayElements = appCore->getOverlayElements();
    lua_pushnil(l);
    const auto &OverlayElementMap = appCore->scriptMaps().OverlayElementMap;
    while (lua_next(l, -2) != 0)
    {
        std::string_view key;
        if (lua_isstring(l, -2))
        {
            key = lua_tostring(l, -2);
        }
        else
        {
            Celx_DoError(l, "Keys in table-argument to celestia:setoverlayelements() must be strings");
            return 0;
        }

        bool value = false;
        if (lua_isboolean(l, -1))
        {
            value = lua_toboolean(l, -1) != 0;
        }
        else
        {
            Celx_DoError(l, "Values in table-argument to celestia:setoverlayelements() must be boolean");
            return 0;
        }

        if (auto it = OverlayElementMap.find(key); it == OverlayElementMap.end())
        {
            GetLogger()->warn("Unknown key: {}\n", key);
        }
        else
        {
            auto element = static_cast<HudElements>(it->second);
            if (value)
                overlayElements |= element;
            else
                overlayElements &= ~element;
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
    const auto overlayElements = appCore->getOverlayElements();
    for (const auto& oem : appCore->scriptMaps().OverlayElementMap)
    {
        lua_pushlstring(l, oem.first.data(), oem.first.size());
        lua_pushboolean(l, util::is_set(overlayElements, static_cast<HudElements>(oem.second)));
        lua_settable(l,-3);
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
    const CelestiaCore* appCore = this_celestia(l);

    const Color& color = appCore->getTextColor();
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
        return 0;
    }

    Color* color = nullptr;
    std::string_view key = lua_tostring(l, 2);
    const auto &LabelColorMap = this_celestia(l)->scriptMaps().LabelColorMap;
    if (auto it = LabelColorMap.find(key); it == LabelColorMap.end())
        GetLogger()->warn("Unknown label style: {}\n", key);
    else
        color = it->second;

    double red     = Celx_SafeGetNumber(l, 3, AllErrors, "setlabelcolor: color values must be numbers");
    double green   = Celx_SafeGetNumber(l, 4, AllErrors, "setlabelcolor: color values must be numbers");
    double blue    = Celx_SafeGetNumber(l, 5, AllErrors, "setlabelcolor: color values must be numbers");

    // opacity currently not settable
    double opacity = 1.0;

    if (color != nullptr)
    {
        *color = Color((float) red, (float) green, (float) blue, (float) opacity);
    }

    return 1;
}


static int celestia_getlabelcolor(lua_State* l)
{
    Celx_CheckArgs(l, 2, 2, "One argument expected for celestia:getlabelcolor()");
    std::string_view key = Celx_SafeGetString(l, 2, AllErrors, "Argument to celestia:getlabelcolor() must be a string");

    Color* labelColor = nullptr;
    const auto &LabelColorMap = this_celestia(l)->scriptMaps().LabelColorMap;
    if (auto it = LabelColorMap.find(key); it == LabelColorMap.end())
    {
        GetLogger()->error("Unknown label style: {}\n", key);
        return 0;
    }
    else
    {
        labelColor = it->second;
    }

    lua_pushnumber(l, labelColor->red());
    lua_pushnumber(l, labelColor->green());
    lua_pushnumber(l, labelColor->blue());

    return 3;
}


static int celestia_setlinecolor(lua_State* l)
{
    Celx_CheckArgs(l, 5, 5, "Four arguments expected for celestia:setlinecolor()");
    if (!lua_isstring(l, 2))
    {
        Celx_DoError(l, "First argument to celestia:setlinecolor() must be a string");
        return 0;
    }

    Color* color = nullptr;
    std::string_view key = lua_tostring(l, 2);
    const auto &LineColorMap = this_celestia(l)->scriptMaps().LineColorMap;
    if (auto it = LineColorMap.find(key); it == LineColorMap.end())
        GetLogger()->warn("Unknown line style: {}\n", key);
    else
        color = it->second;

    double red     = Celx_SafeGetNumber(l, 3, AllErrors, "setlinecolor: color values must be numbers");
    double green   = Celx_SafeGetNumber(l, 4, AllErrors, "setlinecolor: color values must be numbers");
    double blue    = Celx_SafeGetNumber(l, 5, AllErrors, "setlinecolor: color values must be numbers");

    // opacity currently not settable
    double opacity = 1.0;

    if (color != nullptr)
    {
        *color = Color((float) red, (float) green, (float) blue, (float) opacity);
    }

    return 1;
}


static int celestia_getlinecolor(lua_State* l)
{
    Celx_CheckArgs(l, 2, 2, "One argument expected for celestia:getlinecolor()");
    std::string_view key = Celx_SafeGetString(l, 2, AllErrors, "Argument to celestia:getlinecolor() must be a string");

    const auto &LineColorMap = this_celestia(l)->scriptMaps().LineColorMap;
    const auto it = LineColorMap.find(key);
    if (it == LineColorMap.end())
    {
        GetLogger()->error("Unknown line style: {}\n", key);
        return 0;
    }

    lua_pushnumber(l, it->second->red());
    lua_pushnumber(l, it->second->green());
    lua_pushnumber(l, it->second->blue());

    return 3;
}


static int celestia_setfaintestvisible(lua_State* l)
{
    Celx_CheckArgs(l, 2, 2, "One argument expected for celestia:setfaintestvisible()");
    CelestiaCore* appCore = this_celestia(l);
    float faintest = (float)Celx_SafeGetNumber(l, 2, AllErrors, "Argument to celestia:setfaintestvisible() must be a number");
    if (util::is_set(appCore->getRenderer()->getRenderFlags(), RenderFlags::ShowAutoMag))
    {
        faintest = min(12.0f, max(6.0f, faintest));
        appCore->getRenderer()->setFaintestAM45deg(faintest);
        appCore->setFaintestAutoMag();
    }
    else
    {
        faintest = min(15.0f, max(1.0f, faintest));
        appCore->setFaintest(faintest);
        appCore->notifyWatchers(CelestiaCore::FaintestChanged);
    }

    return 0;
}

static int celestia_getfaintestvisible(lua_State* l)
{
    Celx_CheckArgs(l, 1, 1, "No arguments expected for celestia:getfaintestvisible()");
    if (const CelestiaCore* appCore = this_celestia(l);
        util::is_set(appCore->getRenderer()->getRenderFlags(), RenderFlags::ShowAutoMag))
    {
        lua_pushnumber(l, appCore->getRenderer()->getFaintestAM45deg());
    }
    else
    {
        lua_pushnumber(l, appCore->getSimulation()->getFaintestVisible());
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
    if (o == nullptr)
        lua_pushnil(l);
    else
        observer_new(l, o);

    return 1;
}

static int celestia_getobservers(lua_State* l)
{
    Celx_CheckArgs(l, 1, 1, "No arguments expected for celestia:getobservers()");
    const CelestiaCore* appCore = this_celestia(l);

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
        return 0;
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
    if (sel != nullptr)
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

    if (sel != nullptr)
    {
        celestia::MarkerRepresentation markerRep(celestia::MarkerRepresentation::Diamond);
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

    if (sel != nullptr)
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

static int celestia_pause(lua_State* l)
{
    Celx_CheckArgs(l, 1, 2, "No or one argument expected to function celestia:paused");

    CelestiaCore* appCore = this_celestia(l);
    bool cur_state = appCore->getSimulation()->getPauseState();
    bool new_state;

    if (lua_type(l, 2) != LUA_TNONE) // An argument passed
    {
        if (!lua_isboolean(l, -1))
        {
            Celx_DoError(l, "Value passed to celestia:paused must be boolean");
            return 0;
        }
        // set pause state according to a passed value
        new_state = lua_toboolean(l, -1);
    }
    else
    {
        // toggle the current pause state
        new_state = !cur_state;
    }

    // set a new state
    appCore->getSimulation()->setPauseState(new_state);
    // and return the previous one
    lua_pushboolean(l, cur_state);

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
    if (appCore == nullptr)
    {
        Celx_DoError(l, "Bad celestia object!");
        return 0;
    }

    auto i = (uint32_t) lua_tonumber(l, lua_upvalueindex(2));
    Universe* u = appCore->getSimulation()->getUniverse();

    if (i < u->getStarCatalog()->size())
    {
        // Increment the counter
        lua_pushnumber(l, i + 1);
        lua_replace(l, lua_upvalueindex(2));

        Star* star = u->getStarCatalog()->getStar(i);
        if (star == nullptr)
            lua_pushnil(l);
        else
            object_new(l, Selection(star));

        return 1;
    }

    // Return nil when we've enumerated all the stars
    return 0;
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
    if (appCore == nullptr)
    {
        Celx_DoError(l, "Bad celestia object!");
        return 0;
    }

    auto i = (uint32_t) lua_tonumber(l, lua_upvalueindex(2));
    Universe* u = appCore->getSimulation()->getUniverse();

    if (i < u->getDSOCatalog()->size())
    {
        // Increment the counter
        lua_pushnumber(l, i + 1);
        lua_replace(l, lua_upvalueindex(2));

        DeepSkyObject* dso = u->getDSOCatalog()->getDSO(i);
        if (dso == nullptr)
            lua_pushnil(l);
        else
            object_new(l, Selection(dso));

        return 1;
    }

    // Return nil when we've enumerated all the DSOs
    return 0;
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
    ambientLightLevel = std::clamp(ambientLightLevel, 0.0, 1.0);

    if (renderer != nullptr)
        renderer->setAmbientLightLevel(static_cast<float>(ambientLightLevel));
    appCore->notifyWatchers(CelestiaCore::AmbientLightChanged);

    return 0;
}

static int celestia_getambient(lua_State* l)
{
    Celx_CheckArgs(l, 1, 1, "No argument expected in celestia:setambient");
    const CelestiaCore* appCore = this_celestia(l);

    const Renderer* renderer = appCore->getRenderer();
    if (renderer == nullptr)
    {
        Celx_DoError(l, "Internal Error: renderer is nullptr!");
        return 0;
    }

    lua_pushnumber(l, renderer->getAmbientLightLevel());
    return 1;
}

static int celestia_settintsaturation(lua_State* l)
{
    Celx_CheckArgs(l, 2, 2, "One argument expected in celestia:settintsaturation");
    CelestiaCore* appCore = this_celestia(l);

    Renderer* renderer = appCore->getRenderer();
    double tintSaturation = Celx_SafeGetNumber(l, 2, AllErrors, "Argument to celestia:settintsaturation must be a number");
    tintSaturation = std::clamp(tintSaturation, 0.0, 1.0);

    if (renderer != nullptr)
        renderer->setTintSaturation(static_cast<float>(tintSaturation));
    appCore->notifyWatchers(CelestiaCore::TintSaturationChanged);

    return 0;
}

static int celestia_gettintsaturation(lua_State* l)
{
    Celx_CheckArgs(l, 1, 1, "No argument expected in celestia:gettintsaturation");
    const CelestiaCore* appCore = this_celestia(l);

    const Renderer* renderer = appCore->getRenderer();
    if (renderer == nullptr)
    {
        Celx_DoError(l, "Internal Error: renderer is nullptr!");
        return 0;
    }

    lua_pushnumber(l, renderer->getTintSaturation());
    return 1;
}

static int celestia_setminorbitsize(lua_State* l)
{
    Celx_CheckArgs(l, 2, 2, "One argument expected in celestia:setminorbitsize");
    CelestiaCore* appCore = this_celestia(l);

    double orbitSize = Celx_SafeGetNumber(l, 2, AllErrors, "Argument to celestia:setminorbitsize() must be a number");
    Renderer* renderer = appCore->getRenderer();
    if (renderer == nullptr)
    {
        Celx_DoError(l, "Internal Error: renderer is nullptr!");
        return 0;
    }

    orbitSize = max(0.0, orbitSize);
    renderer->setMinimumOrbitSize((float)orbitSize);
    return 0;
}

static int celestia_getminorbitsize(lua_State* l)
{
    Celx_CheckArgs(l, 1, 1, "No argument expected in celestia:getminorbitsize");
    CelestiaCore* appCore = this_celestia(l);

    Renderer* renderer = appCore->getRenderer();
    if (renderer == nullptr)
    {
        Celx_DoError(l, "Internal Error: renderer is nullptr!");
        return 0;
    }

    lua_pushnumber(l, renderer->getMinimumOrbitSize());
    return 1;
}

static int celestia_setstardistancelimit(lua_State* l)
{
    Celx_CheckArgs(l, 2, 2, "One argument expected in celestia:setstardistancelimit");
    CelestiaCore* appCore = this_celestia(l);

    double distanceLimit = Celx_SafeGetNumber(l, 2, AllErrors, "Argument to celestia:setstardistancelimit() must be a number");
    Renderer* renderer = appCore->getRenderer();
    if (renderer == nullptr)
    {
        Celx_DoError(l, "Internal Error: renderer is nullptr!");
        return 0;
    }

    renderer->setDistanceLimit((float)distanceLimit);
    return 0;
}

static int celestia_getstardistancelimit(lua_State* l)
{
    Celx_CheckArgs(l, 1, 1, "No argument expected in celestia:getstardistancelimit");
    CelestiaCore* appCore = this_celestia(l);

    Renderer* renderer = appCore->getRenderer();
    if (renderer == nullptr)
    {
        Celx_DoError(l, "Internal Error: renderer is nullptr!");
        return 0;
    }

    lua_pushnumber(l, renderer->getDistanceLimit());
    return 1;
}

static int celestia_getstarstyle(lua_State* l)
{
    Celx_CheckArgs(l, 1, 1, "No argument expected in celestia:getstarstyle");
    CelestiaCore* appCore = this_celestia(l);

    Renderer* renderer = appCore->getRenderer();
    if (renderer == nullptr)
    {
        Celx_DoError(l, "Internal Error: renderer is nullptr!");
        return 0;
    }

    StarStyle starStyle = renderer->getStarStyle();
    switch (starStyle)
    {
    case StarStyle::FuzzyPointStars:
        lua_pushstring(l, "fuzzy"); break;
    case StarStyle::PointStars:
        lua_pushstring(l, "point"); break;
    case StarStyle::ScaledDiscStars:
        lua_pushstring(l, "disc"); break;
    default:
        lua_pushstring(l, "invalid starstyle");
    };
    return 1;
}

static int celestia_setstarstyle(lua_State* l)
{
    Celx_CheckArgs(l, 2, 2, "One argument expected in celestia:setstarstyle");
    CelestiaCore* appCore = this_celestia(l);

    std::string_view starStyle = Celx_SafeGetString(l, 2, AllErrors, "Argument to celestia:setstarstyle must be a string");
    Renderer* renderer = appCore->getRenderer();
    if (renderer == nullptr)
    {
        Celx_DoError(l, "Internal Error: renderer is nullptr!");
        return 0;
    }

    if (starStyle == "fuzzy"sv)
        renderer->setStarStyle(StarStyle::FuzzyPointStars);
    else if (starStyle == "point"sv)
        renderer->setStarStyle(StarStyle::PointStars);
    else if (starStyle == "disc"sv)
        renderer->setStarStyle(StarStyle::ScaledDiscStars);
    else
       Celx_DoError(l, "Invalid starstyle");

    appCore->notifyWatchers(CelestiaCore::RenderFlagsChanged);
    return 0;
}

// -----------------------------------------------------------------------------
// Star Color

static int celestia_getstarcolor(lua_State* l)
{
    Celx_CheckArgs(l, 1, 1, "No argument expected in celestia:getstarcolor");
    CelestiaCore* appCore = this_celestia(l);

    Renderer* renderer = appCore->getRenderer();
    if (renderer == nullptr)
    {
        Celx_DoError(l, "Internal Error: renderer is nullptr!");
        return 0;
    }

    switch (renderer->getStarColorTable())
    {
    case ColorTableType::Enhanced:
        lua_pushstring(l, "enhanced");
        break;
    case ColorTableType::Blackbody_D65:
        lua_pushstring(l, "blackbody_d65");
        break;
    case ColorTableType::SunWhite:
        lua_pushstring(l, "sunwhite");
        break;
    case ColorTableType::VegaWhite:
        lua_pushstring(l, "vegawhite");
        break;
    default:
        lua_pushstring(l, "invalid starcolor");
        break;
    }

    return 1;
}

static int celestia_setstarcolor(lua_State* l)
{
    Celx_CheckArgs(l, 2, 2, "One argument expected in celestia:setstarcolor");
    CelestiaCore* appCore = this_celestia(l);

    std::string_view starColor = Celx_SafeGetString(l, 2, AllErrors, "Argument to celestia:setstarcolor must be a string");
    Renderer* renderer = appCore->getRenderer();
    if (renderer == nullptr)
    {
        Celx_DoError(l, "Internal Error: renderer is nullptr!");
        return 0;
    }

    if (starColor == "blackbody_d65"sv)
        renderer->setStarColorTable(ColorTableType::Blackbody_D65);
    else if (starColor == "enhanced"sv)
        renderer->setStarColorTable(ColorTableType::Enhanced);
    else if (starColor == "sunwhite"sv)
        renderer->setStarColorTable(ColorTableType::SunWhite);
    else if (starColor == "vegawhite"sv)
        renderer->setStarColorTable(ColorTableType::VegaWhite);
    else
        Celx_DoError(l, "Invalid starcolor");
    appCore->notifyWatchers(CelestiaCore::RenderFlagsChanged);

    return 0;
}

// -----------------------------------------------------------------------------

static int celestia_gettextureresolution(lua_State* l)
{
    Celx_CheckArgs(l, 1, 1, "No argument expected in celestia:gettextureresolution");
    CelestiaCore* appCore = this_celestia(l);

    Renderer* renderer = appCore->getRenderer();
    if (renderer == nullptr)
    {
        Celx_DoError(l, "Internal Error: renderer is nullptr!");
        return 0;
    }

    lua_pushnumber(l, static_cast<int>(renderer->getResolution()));

    return 1;
}

static int celestia_settextureresolution(lua_State* l)
{
    Celx_CheckArgs(l, 2, 2, "One argument expected in celestia:settextureresolution");
    CelestiaCore* appCore = this_celestia(l);

    auto textureResValue = Celx_SafeGetNumber(l, 2, AllErrors, "Argument to celestia:settextureresolution must be a number");
    Renderer* renderer = appCore->getRenderer();
    if (renderer == nullptr)
    {
        Celx_DoError(l, "Internal Error: renderer is nullptr!");
        return 0;
    }

    if (textureResValue < 0.0 || textureResValue >= 3.0)
    {
        Celx_DoError(l, "Texture resolution out of range");
        return 0;
    }

    switch (static_cast<int>(textureResValue))
    {
        case 0:
            renderer->setResolution(TextureResolution::lores);
            break;
        case 1:
            renderer->setResolution(TextureResolution::medres);
            break;
        case 2:
            renderer->setResolution(TextureResolution::hires);
            break;
        default:
            assert(0);
            break;
    }

    appCore->notifyWatchers(CelestiaCore::RenderFlagsChanged);

    return 0;
}

static int celestia_getstar(lua_State* l)
{
    Celx_CheckArgs(l, 2, 2, "One argument expected to function celestia:getstar");

    CelestiaCore* appCore = this_celestia(l);
    double starIndex = Celx_SafeGetNumber(l, 2, AllErrors, "First arg to celestia:getstar must be a number");
    Universe* u = appCore->getSimulation()->getUniverse();
    Star* star = u->getStarCatalog()->find((uint32_t) starIndex);
    if (star == nullptr)
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
    DeepSkyObject* dso = u->getDSOCatalog()->find((uint32_t) dsoIndex);
    if (dso == nullptr)
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

    vector_new(l, Vector3d(x,y,z));

    return 1;
}

static int celestia_newposition(lua_State* l)
{
    Celx_CheckArgs(l, 4, 4, "Expected 3 arguments for celestia:newposition");
    // for error checking only:
    this_celestia(l);
    R128 components[3];
    for (int i = 0; i < 3; i++)
    {
        if (lua_isnumber(l, i+2))
        {
            double v = lua_tonumber(l, i+2);
            components[i] = R128(v);
        }
        else if (lua_isstring(l, i+2))
        {
            components[i] = util::DecodeFromBase64(lua_tostring(l, i+2));
        }
        else
        {
            Celx_DoError(l, "Arguments to celestia:newposition must be either numbers or strings");
            return 0;
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
        Quaterniond q(w, x, y, z);
        rotation_new(l, q);
    }
    else
    {
        auto v = to_vector(l, 2);
        if (v == nullptr)
        {
            Celx_DoError(l, "newrotation: first argument must be a vector");
            return 0;
        }
        double angle = Celx_SafeGetNumber(l, 3, AllErrors, "second argument to celestia:newrotation must be a number");
        Quaterniond q(AngleAxisd(angle, v->normalized()));
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
    Selection* ref = nullptr;
    Selection* target = nullptr;

    if (coordSys == ObserverFrame::CoordinateSystem::Universal)
    {
        frame_new(l, ObserverFrame());
    }
    else if (coordSys == ObserverFrame::CoordinateSystem::PhaseLock)
    {
        if (argc >= 4)
        {
            ref = to_object(l, 3);
            target = to_object(l, 4);
        }

        if (ref == nullptr || target == nullptr)
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
        if (ref == nullptr)
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
        return 0;
    }

    auto mode = appCore->getTextEnterMode();

    if (lua_toboolean(l, 2))
    {
        // Check for existence of charEntered:
        lua_getglobal(l, KbdCallback);
        if (lua_isnil(l, -1))
        {
            Celx_DoError(l, "script requested keyboard, but did not provide callback");
        }
        lua_remove(l, -1);

        mode = mode | Hud::TextEnterMode::PassToScript;
    }
    else
    {
        mode = mode & ~Hud::TextEnterMode::PassToScript;
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
        return 0;
    }

    if (!lua_isfunction(l, 3) && !lua_isnil(l, 3))
    {
        Celx_DoError(l, "Second argument for celestia:registereventhandler must be a function or nil");
        return 0;
    }

    lua_pushstring(l, EventHandlers);
    lua_gettable(l, LUA_REGISTRYINDEX);
    if (lua_isnil(l, -1))
    {
        // This should never happen--the table should be created when a new Celestia Lua
        // state is initialized.
        Celx_DoError(l, "Event handler table not created");
        return 0;
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
        return 0;
    }

    lua_pushstring(l, EventHandlers);
    lua_gettable(l, LUA_REGISTRYINDEX);
    if (lua_isnil(l, -1))
    {
        // This should never happen--the table should be created when a new Celestia Lua
        // state is initialized.
        Celx_DoError(l, "Event handler table not created");
        return 0;
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
    if (filetype == nullptr)
        filetype = "png";

    // Let the script safely contribute one part of the filename:
    const char* fileid_ptr = Celx_SafeGetString(l, 3, WrongType, "Second argument to celestia:takescreenshot must be a string");
    if (fileid_ptr == nullptr)
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

    luastate->screenshotCount++;
    bool success = false;
    string filenamestem;
    filenamestem = fmt::format("screenshot-{}{:06i}", fileid, luastate->screenshotCount);

    fs::path path = appCore->getConfig()->paths.scriptScreenshotDirectory;
    fs::path filepath = path / fmt::format("{}.{}", filenamestem, filetype);
    success = appCore->saveScreenShot(filepath);
    lua_pushboolean(l, success);

    // no matter how long it really took, make it look like 0.1s to timeout check:
    luastate->timeout = luastate->getTime() + timeToTimeout - 0.1;
    return 1;
}

static int celestia_createcelscript(lua_State* l)
{
    Celx_CheckArgs(l, 2, 2, "Need one argument for celestia:createcelscript()");
    const char* scripttext = Celx_SafeGetString(l, 2, AllErrors, "Argument to celestia:createcelscript() must be a string");
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
    const char* scriptfile = Celx_SafeGetString(l, 2, AllErrors, "Argument to celestia:runscript must be a string");

    fs::path base_dir = GetScriptPath(l);
    CelestiaCore* appCore = this_celestia(l);
    appCore->runScript(base_dir / scriptfile);
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

    const char* url = Celx_SafeGetString(l, 2, AllErrors, "First argument to celestia:seturl must be a string");
    const Observer* obs = to_observer(l, 3);
    if (obs == nullptr)
        obs = appCore->getSimulation()->getActiveObserver();
    const celestia::View* view = getViewByObserver(appCore, obs);
    appCore->setActiveView(view);

    appCore->goToUrl(url);

    return 0;
}

static int celestia_geturl(lua_State* l)
{
    Celx_CheckArgs(l, 1, 2, "None or one argument expected for celestia:geturl");
    CelestiaCore* appCore = this_celestia(l);

    const Observer* obs = to_observer(l, 2);
    if (obs == nullptr)
        obs = appCore->getSimulation()->getActiveObserver();
    const celestia::View* view = getViewByObserver(appCore, obs);
    appCore->setActiveView(view);

    CelestiaState appState(appCore);
    appState.captureState();

    Url url(appState);
    lua_pushstring(l, url.getAsString().c_str());

    return 1;
}


static int celestia_overlay(lua_State* l)
{
    Celx_CheckArgs(l, 2, 7, "One to Six arguments expected to function celestia:overlay");

    CelestiaCore* appCore = this_celestia(l);
    float duration = Celx_SafeGetNumber(l, 2, WrongType, "First argument to celestia:overlay must be a number (duration)", 3.0);
    float xoffset = Celx_SafeGetNumber(l, 3, WrongType, "Second argument to celestia:overlay must be a number (xoffset)", 0.0);
    float yoffset = Celx_SafeGetNumber(l, 4, WrongType, "Third argument to celestia:overlay must be a number (yoffset)", 0.0);
    float alpha = Celx_SafeGetNumber(l, 5, WrongType, "Fourth argument to celestia:overlay must be a number (alpha)", 1.0);
    const char* filename = Celx_SafeGetString(l, 6, AllErrors, "Fifth argument to celestia:overlay must be a string (filename)");
    bool fitscreen;
    if (lua_isboolean(l, 7))
        fitscreen = lua_toboolean(l, 7);
    else
        fitscreen = (bool) Celx_SafeGetNumber(l, 7, WrongType, "Sixth argument to celestia:overlay must be a number or a boolean(fitscreen)", 0);

    auto image = std::make_unique<OverlayImage>(filename, appCore->getRenderer());
    image->setDuration(duration);
    image->setFadeAfter(duration); // FIXME
    image->setOffset(xoffset, yoffset);
    image->setColor({Color::White, alpha}); // FIXME
    image->fitScreen(fitscreen);

    appCore->setScriptImage(std::move(image));

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


#ifdef USE_MINIAUDIO
static int celestia_getchannel(lua_State *l, const char* errorMessage)
{
    if (!lua_isnumber(l, 2))
    {
        Celx_DoError(l, errorMessage);
        return 0; // we do not get here due to longjmp in lua_error
    }
    return max(static_cast<int>(Celx_SafeGetNumber(l, 2, AllErrors, errorMessage, static_cast<lua_Number>(defaultAudioChannel))), minAudioChannel);
}
#endif

static int celestia_play(lua_State *l)
{
#ifdef USE_MINIAUDIO
    Celx_CheckArgs(l, 3, 7, "Two to six arguments expected to function celestia:play");
    int channel = celestia_getchannel(l, "First argument for celestia:play must be a number");

    CelestiaCore* appCore = this_celestia(l);
    auto volume = static_cast<float>(Celx_SafeGetNumber(l, 3, AllErrors, "Second argument to celestia:play must be a number (volume)", static_cast<lua_Number>(defaultAudioVolume)));
    float pan = clamp(static_cast<float>(Celx_SafeGetNumber(l, 4, WrongType, "Third argument to celestia:play must be a number (pan)", static_cast<lua_Number>(defaultAudioPan))), minAudioPan, maxAudioPan);
    bool loopSet = !lua_isnil(l, 5) && lua_isnumber(l, 5);
    bool loop = loopSet && static_cast<int>(Celx_SafeGetNumber(l, 5, WrongType, "Fourth argument to celestia:play must be a number (loop)", 0.0)) == 1;
    const char* filename = Celx_SafeGetString(l, 6, WrongType, "Fifth argument to celestia:play must be a string (filename)");
    bool nopause = static_cast<int>(Celx_SafeGetNumber(l, 7, WrongType, "Sixth argument to celestia:play must be a number (nopause)", 0.0)) == 1;

    if (!filename)
    {
        // filename not set, only try to set values
        if (volume >= 0)
            appCore->setAudioVolume(channel, clamp(volume, minAudioVolume, maxAudioVolume));
        appCore->setAudioPan(channel, pan);
        if (loopSet)
            appCore->setAudioLoop(channel, loop);
    }
    else if (*filename == '\0')
    {
        // empty filename
        appCore->stopAudio(channel);
    }
    else
    {
        appCore->playAudio(channel, filename, 0.0, clamp(volume, minAudioVolume, maxAudioVolume), pan, loop, nopause);
    }
#else
    Celx_DoError(l, "Audio playback is not supported");
#endif
    return 0;
}

static int celestia_isplayingaudio(lua_State* l)
{
#ifdef USE_MINIAUDIO
    Celx_CheckArgs(l, 2, 2, "Function celestia:isplayingaudio requires one argument");
    int channel = celestia_getchannel(l, "First argument for celestia:isplayingaudio must be a number");
    CelestiaCore* appCore = this_celestia(l);
    lua_pushboolean(l, appCore->isPlayingAudio(channel));
#else
    Celx_DoError(l, "Audio playback is not supported");
    lua_pushboolean(l, false);
#endif
    return 1;
}

static int celestia_playaudio(lua_State* l)
{
#ifdef USE_MINIAUDIO
    Celx_CheckArgs(l, 3, 7, "Function celestia:playaudio requires two to seven arguments");
    int channel = celestia_getchannel(l, "First argument for celestia:playaudio must be a number");

    const char* path = Celx_SafeGetString(l, 3, AllErrors, "Second argument to celestia:playaudio must be a string");
    if (path == nullptr)
    {
        lua_pushboolean(l, false);
        return 1;
    }

    double startTime = max(Celx_SafeGetNumber(l, 4, WrongType, "Third argument to celestia:playaudio must be a number", 0.0), 0.0);
    float volume = clamp(static_cast<float>(Celx_SafeGetNumber(l, 5, WrongType, "Fourth argument to celestia:playaudio must be a number", static_cast<lua_Number>(defaultAudioVolume))), minAudioVolume, maxAudioVolume);
    float pan = clamp(static_cast<float>(Celx_SafeGetNumber(l, 6, WrongType, "Fifth argument to celestia:playaudio must be a number", static_cast<lua_Number>(defaultAudioPan))), minAudioPan, maxAudioPan);
    bool loop = Celx_SafeGetBoolean(l, 7, WrongType, "Sixth argument to celestia:playaudio must be a boolean", false);
    bool nopause = Celx_SafeGetBoolean(l, 7, WrongType, "Seventh argument to celestia:playaudio must be a number(nopause)", false);
    CelestiaCore* appCore = this_celestia(l);
    lua_pushboolean(l, appCore->playAudio(channel, path, startTime, volume, pan, loop, nopause));
#else
    Celx_DoError(l, "Audio playback is not supported");
    lua_pushboolean(l, false);
#endif
    return 1;
}

static int celestia_resumeaudio(lua_State* l)
{
#ifdef USE_MINIAUDIO
    Celx_CheckArgs(l, 2, 2, "Function celestia:resumeaudio requires one argument");
    int channel = celestia_getchannel(l, "First argument for celestia:resumeaudio must be a number");
    CelestiaCore* appCore = this_celestia(l);
    lua_pushboolean(l, appCore->resumeAudio(channel));
#else
    Celx_DoError(l, "Audio playback is not supported");
    lua_pushboolean(l, false);
#endif
    return 1;
}

static int celestia_pauseaudio(lua_State* l)
{
#ifdef USE_MINIAUDIO
    Celx_CheckArgs(l, 2, 2, "Function celestia:pauseaudio requires one argument");
    int channel = celestia_getchannel(l, "First argument for celestia:pauseaudio must be a number");
    CelestiaCore* appCore = this_celestia(l);
    appCore->pauseAudio(channel);
#else
    Celx_DoError(l, "Audio playback is not supported");
#endif
    return 0;
}

static int celestia_stopaudio(lua_State* l)
{
#ifdef USE_MINIAUDIO
    Celx_CheckArgs(l, 2, 2, "Function celestia:stopaudio requires one argument");
    int channel = celestia_getchannel(l, "First argument for celestia:stopaudio must be a number");
    CelestiaCore* appCore = this_celestia(l);
    appCore->stopAudio(channel);
#else
    Celx_DoError(l, "Audio playback is not supported");
#endif
    return 0;
}

static int celestia_seekaudio(lua_State* l)
{
#ifdef USE_MINIAUDIO
    Celx_CheckArgs(l, 3, 3, "Function celestia:seekaudio requires two arguments");
    int channel = celestia_getchannel(l, "First argument for celestia:seekaudio must be a number");

    if (!lua_isnumber(l, 3))
    {
        Celx_DoError(l, "Second argument for celestia:seekaudio must be a number");
        lua_pushboolean(l, false);
        return 1;
    }

    double time = max(Celx_SafeGetNumber(l, 3, AllErrors, "Second argument for celestia:seekaudio must be a number"), 0.0);
    CelestiaCore* appCore = this_celestia(l);
    lua_pushboolean(l, appCore->seekAudio(channel, time));
#else
    Celx_DoError(l, "Audio playback is not supported");
    lua_pushboolean(l, false);
#endif
    return 1;
}

static int celestia_setaudiovolume(lua_State* l)
{
#ifdef USE_MINIAUDIO
    Celx_CheckArgs(l, 3, 3, "Function celestia:setaudiovolume requires two arguments");
    int channel = celestia_getchannel(l, "First argument for celestia:setaudiovolume must be a number");

    if (!lua_isnumber(l, 3))
    {
        Celx_DoError(l, "Second argument for celestia:setaudiovolume must be a number");
        return 0;
    }

    float volume = clamp(static_cast<float>(Celx_SafeGetNumber(l, 3, WrongType, "Second argument for celestia:setaudiovolume must be a number", static_cast<lua_Number>(defaultAudioVolume))), minAudioVolume, maxAudioVolume);
    CelestiaCore* appCore = this_celestia(l);
    appCore->setAudioVolume(channel, volume);
#else
    Celx_DoError(l, "Audio playback is not supported");
#endif
    return 0;
}

static int celestia_setaudiopan(lua_State* l)
{
#ifdef USE_MINIAUDIO
    Celx_CheckArgs(l, 3, 3, "Function celestia:setaudiopan requires two arguments");
    int channel = celestia_getchannel(l, "First argument for celestia:setaudiopan must be a number");

    if (!lua_isnumber(l, 3))
    {
        Celx_DoError(l, "Second argument for celestia:setaudiopan must be a number");
        return 0;
    }

    float pan = clamp(static_cast<float>(Celx_SafeGetNumber(l, 3, WrongType, "Second argument for celestia:setaudiopan must be a number", static_cast<lua_Number>(defaultAudioPan))), minAudioPan, maxAudioPan);
    CelestiaCore* appCore = this_celestia(l);
    appCore->setAudioPan(channel, pan);
#else
    Celx_DoError(l, "Audio playback is not supported");
#endif
    return 0;
}

static int celestia_setaudioloop(lua_State* l)
{
#ifdef USE_MINIAUDIO
    Celx_CheckArgs(l, 3, 3, "Function celestia:setaudioloop requires two arguments");
    int channel = celestia_getchannel(l, "First argument for celestia:setaudioloop must be a number");

    if (!lua_isboolean(l, 3))
    {
        Celx_DoError(l, "Second argument for celestia:setaudioloop must be a boolean");
        lua_pushboolean(l, false);
        return 0;
    }

    bool loop = Celx_SafeGetBoolean(l, 3, WrongType, "Second argument for celestia:setaudioloop must be a boolean", false);
    CelestiaCore* appCore = this_celestia(l);
    appCore->setAudioLoop(channel, loop);
#else
    Celx_DoError(l, "Audio playback is not supported");
#endif
    return 0;
}

static int celestia_setaudionopause(lua_State* l)
{
#ifdef USE_MINIAUDIO
    Celx_CheckArgs(l, 3, 3, "Function celestia:setaudionopause requires two arguments");
    int channel = celestia_getchannel(l, "First argument for celestia:setaudionopause must be a number");

    if (!lua_isboolean(l, 3))
    {
        Celx_DoError(l, "Second argument for celestia:setaudionopause must be a boolean");
        lua_pushboolean(l, false);
        return 0;
    }

    bool nopause = Celx_SafeGetBoolean(l, 3, WrongType, "Second argument for celestia:setaudionopause must be a boolean", false);
    CelestiaCore* appCore = this_celestia(l);
    appCore->setAudioNoPause(channel, nopause);
#else
    Celx_DoError(l, "Audio playback is not supported");
#endif
    return 0;
}

static int celestia_version(lua_State* l)
{
    lua_pushstring(l, VERSION);
    return 1;
}

void CreateCelestiaMetaTable(lua_State* l)
{
    Celx_CreateClassMetatable(l, Celx_Celestia);

    Celx_RegisterMethod(l, "__tostring", celestia_tostring);
    Celx_RegisterMethod(l, "flash", celestia_flash);
    Celx_RegisterMethod(l, "print", celestia_print);
    Celx_RegisterMethod(l, "printatpixel", celestia_printatpixel);
    Celx_RegisterMethod(l, "gettextwidth", celestia_gettextwidth);
    Celx_RegisterMethod(l, "show", celestia_show);
    Celx_RegisterMethod(l, "setaltazimuthmode", celestia_setaltazimuthmode);
    Celx_RegisterMethod(l, "getaltazimuthmode", celestia_getaltazimuthmode);
    Celx_RegisterMethod(l, "getscreendpi", celestia_getscreendpi);
    Celx_RegisterMethod(l, "setscreendpi", celestia_setscreendpi);
    Celx_RegisterMethod(l, "hide", celestia_hide);
    Celx_RegisterMethod(l, "getrenderflags", celestia_getrenderflags);
    Celx_RegisterMethod(l, "setrenderflags", celestia_setrenderflags);
    Celx_RegisterMethod(l, "getscreendimension", celestia_getscreendimension);
    Celx_RegisterMethod(l, "getwindowdimension", celestia_getwindowdimension);
    Celx_RegisterMethod(l, "getsafeareainsets", celestia_getsafeareainsets);
    Celx_RegisterMethod(l, "setsafeareainsets", celestia_setsafeareainsets);
    Celx_RegisterMethod(l, "getlayoutdirection", celestia_getlayoutdirection);
    Celx_RegisterMethod(l, "setlayoutdirection", celestia_setlayoutdirection);
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
    Celx_RegisterMethod(l, "pause", celestia_pause);
    Celx_RegisterMethod(l, "synchronizetime", celestia_synchronizetime);
    Celx_RegisterMethod(l, "istimesynchronized", celestia_istimesynchronized);
    Celx_RegisterMethod(l, "gettimescale", celestia_gettimescale);
    Celx_RegisterMethod(l, "settimescale", celestia_settimescale);
    Celx_RegisterMethod(l, "getambient", celestia_getambient);
    Celx_RegisterMethod(l, "setambient", celestia_setambient);
    Celx_RegisterMethod(l, "gettintsaturation", celestia_gettintsaturation);
    Celx_RegisterMethod(l, "settintsaturation", celestia_settintsaturation);
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
    Celx_RegisterMethod(l, "overlay", celestia_overlay);
    Celx_RegisterMethod(l, "verbosity", celestia_verbosity);

    // Compatibility audio playback
    Celx_RegisterMethod(l, "play", celestia_play);

    // Audio playback
    Celx_RegisterMethod(l, "isplayingaudio", celestia_isplayingaudio);
    Celx_RegisterMethod(l, "playaudio", celestia_playaudio);
    Celx_RegisterMethod(l, "resumeaudio", celestia_resumeaudio);
    Celx_RegisterMethod(l, "pauseaudio", celestia_pauseaudio);
    Celx_RegisterMethod(l, "stopaudio", celestia_stopaudio);
    Celx_RegisterMethod(l, "seekaudio", celestia_seekaudio);
    Celx_RegisterMethod(l, "setaudiovolume", celestia_setaudiovolume);
    Celx_RegisterMethod(l, "setaudiopan", celestia_setaudiopan);
    Celx_RegisterMethod(l, "setaudioloop", celestia_setaudioloop);
    Celx_RegisterMethod(l, "setaudionopause", celestia_setaudionopause);

    Celx_RegisterMethod(l, "version", celestia_version);

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
    const CelestiaConfig* config = appCore->getConfig();
    if (auto params = config->configParams.getHash(); params != nullptr)
    {
        const std::string* paramString = params->getString(s);
        if (paramString == nullptr)
            lua_pushstring(l, "");
        else
            lua_pushlstring(l, paramString->data(), paramString->size());
    }
    return 1;
}

static int celestia_loadtexture(lua_State* l)
{
    CelxLua celx(l);

    celx.checkArgs(2, 4, "Need one to three arguments for celestia:loadtexture()");
    std::string_view s = celx.safeGetString(2, AllErrors, "First argument to celestia:loadtexture() must be a string");
    auto argc = lua_gettop(l);
    auto addressMode = Texture::AddressMode::EdgeClamp;
    auto mipMapMode = Texture::MipMapMode::DefaultMipMaps;
    if (argc >= 3)
    {
        std::string_view addressModeString = Celx_SafeGetString(l, 3, AllErrors, "Second argument to celestia:loadtexture must be a string");

        if (addressModeString == "wrap"sv)
            addressMode = Texture::AddressMode::Wrap;
        else if (addressModeString == "borderclamp"sv)
            addressMode = Texture::AddressMode::BorderClamp;
        else if (addressModeString == "edgeclamp"sv)
            addressMode = Texture::AddressMode::EdgeClamp;
        else
           Celx_DoError(l, "Invalid addressMode");
    }
    if (argc >= 4)
    {
        std::string_view mipMapModeString = Celx_SafeGetString(l, 4, AllErrors, "Third argument to celestia:loadtexture must be a string");

        if (mipMapModeString == "default"sv)
            mipMapMode = Texture::MipMapMode::DefaultMipMaps;
        else if (mipMapModeString == "none"sv)
            mipMapMode = Texture::MipMapMode::NoMipMaps;
        else
           Celx_DoError(l, "Invalid mipMapMode");
    }
    fs::path base_dir = GetScriptPath(l);
    auto t = LoadTextureFromFile(base_dir / s, addressMode, mipMapMode);
    if (t == nullptr) return 0;
    return celx.pushClass(t.release());
}

static int celestia_loadfont(lua_State* l)
{
    CelxLua celx(l);

    celx.checkArgs(2, 2, "Need one argument for celestia:loadfont()");
    std::string_view s = celx.safeGetString(2, AllErrors, "Argument to celestia:loadfont() must be a string");
    CelestiaCore* appCore = getAppCore(l, AllErrors);
    auto font = LoadTextureFont(appCore->getRenderer(), s);
    if (font == nullptr) return 0;
    return celx.pushClass(font);
}

std::shared_ptr<TextureFont> getFont(CelestiaCore* appCore)
{
    return appCore->hud->font();
}

static int celestia_getfont(lua_State* l)
{
    CelxLua celx(l);

    celx.checkArgs(1, 1, "No arguments expected to function celestia:getfont");

    CelestiaCore* appCore = getAppCore(l, AllErrors);
    auto font = getFont(appCore);
    if (font == nullptr)
        return 0;
    return celx.pushClass(font);
}

std::shared_ptr<TextureFont> getTitleFont(CelestiaCore* appCore)
{
    return appCore->hud->titleFont();
}

static int celestia_gettitlefont(lua_State* l)
{
    CelxLua celx(l);

    celx.checkArgs(1, 1, "No arguments expected to function celestia:gettitlefont");

    CelestiaCore* appCore = getAppCore(l, AllErrors);
    auto font = getTitleFont(appCore);
    if (font == nullptr)
        return 0;
    return celx.pushClass(font);
}

static int celestia_settimeslice(lua_State* l)
{
    Celx_CheckArgs(l, 2, 2, "One argument required for celestia:settimeslice()");
    //CelestiaCore* appCore = this_celestia(l);

    if (!lua_isnumber(l, 2) && !lua_isnil(l, 2))
    {
        Celx_DoError(l, "Argument for celestia:settimeslice must be a number");
        return 0;
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
    Celx_CheckArgs(l, 2, 2, "One argument required for celestia:setluahook()");
    CelestiaCore* appCore = this_celestia(l);

    if (!lua_istable(l, 2) && !lua_isnil(l, 2))
    {
        Celx_DoError(l, "Argument for celestia:setluahook must be a table or nil");
        return 0;
    }

    LuaState* luastate = getLuaStateObject(l);
    if (luastate != nullptr)
    {
        luastate->setLuaHookEventHandlerEnabled(lua_istable(l, 2));
    }

    lua_pushlightuserdata(l, appCore);
    lua_pushvalue(l, -2);
    lua_settable(l, LUA_REGISTRYINDEX);

    return 0;
}

static int celestia_newcategory(lua_State *l)
{
    CelxLua celx(l);

    constexpr const char emsg[] = "Argument of celestia:newcategory must be a string!";
    const char *name = celx.safeGetString(2, AllErrors, emsg);
    const char *domain = "";
    if (name == nullptr)
    {
        celx.doError(emsg);
        return 0;
    }
    if (celx.isString(2))
        domain = celx.getString(2);
    UserCategoryId c = UserCategory::create(name, UserCategoryId::Invalid, domain);
    if (c == UserCategoryId::Invalid)
        return 0;
    return celx.pushClass(c);
}

static int celestia_findcategory(lua_State *l)
{
    CelxLua celx(l);

    constexpr const char emsg[] = "Argument of celestia:fndcategory must be a string.";
    const char *name = celx.safeGetString(2, AllErrors, emsg);
    if (name == nullptr)
    {
        celx.doError(emsg);
        return 0;
    }
    UserCategoryId c = UserCategory::find(name);
    if (c == UserCategoryId::Invalid)
        return 0;
    return celx.pushClass(c);
}

static int celestia_deletecategory(lua_State *l)
{
    CelxLua celx(l);

    bool ret;
    constexpr const char emsg[] = "Argument of celestia:deletecategory() must be a string or userdata.";
    if (celx.isString(2))
    {
        const char *n = celx.safeGetString(2, AllErrors, emsg);
        if (n == nullptr)
        {
            celx.doError(emsg);
            return 0;
        }
        auto c = UserCategory::find(n);
        ret = UserCategory::destroy(c);
    }
    else
    {
        auto c = *celx.safeGetClass<UserCategoryId>(2, AllErrors, emsg);
        if (c == UserCategoryId::Invalid)
        {
            celx.doError(emsg);
            return 0;
        }
        ret = UserCategory::destroy(c);
    }
    return celx.push(ret);
}

static int celestia_getcategories(lua_State *l)
{
    CelxLua celx(l);

    const auto& set = UserCategory::active();
    return celx.pushIterable<UserCategoryId>(set);
}

static int celestia_getrootcategories(lua_State *l)
{
    CelxLua celx(l);

    const auto& set = UserCategory::roots();
    return celx.pushIterable<UserCategoryId>(set);
}

static int celestia_bindtranslationdomain(lua_State *l)
{
#ifdef ENABLE_NLS
    CelxLua celx(l);

    const char *domain = celx.safeGetNonEmptyString(2, AllErrors, "First argument of celestia:bindtranslationdomain must be domain name string.");
    const char *dir = celx.safeGetString(3, AllErrors, "Second argument of celestia:bindtranslationdomain must be directory name string.");
    const char *newdir = bindtextdomain(domain, dir);
    if (newdir == nullptr)
        return 0;
    return celx.push(newdir);
#else
    return 0;
#endif
}

static int celestia_setasterisms(lua_State *l)
{
    CelxLua celx(l);
    celx.checkArgs(2, 2, "Need one argument for celestia:setasterisms()");
    const char* s = celx.safeGetString(2, AllErrors, "Argument to celestia:setasterisms() must be a string");
    CelestiaCore* appCore = getAppCore(l, AllErrors);

    appCore->loadAsterismsFile(s);

    return 0;
}

void ExtendCelestiaMetaTable(lua_State* l)
{
    CelxLua celx(l);

    celx.pushClassName(Celx_Celestia);
    lua_rawget(l, LUA_REGISTRYINDEX);
    if (lua_type(l, -1) != LUA_TTABLE)
        std::cout << "Metatable for " << CelxLua::classNameForId(Celx_Celestia) << " not found!\n";
    celx.registerMethod("log", celestia_log);
    celx.registerMethod("settimeslice", celestia_settimeslice);
    celx.registerMethod("setluahook", celestia_setluahook);
    celx.registerMethod("getparamstring", celestia_getparamstring);
    celx.registerMethod("getfont", celestia_getfont);
    celx.registerMethod("gettitlefont", celestia_gettitlefont);
    celx.registerMethod("loadtexture", celestia_loadtexture);
    celx.registerMethod("loadfont", celestia_loadfont);
    celx.registerMethod("newcategory", celestia_newcategory);
    celx.registerMethod("findcategory", celestia_findcategory);
    celx.registerMethod("deletecategory", celestia_deletecategory);
    celx.registerMethod("getcategories", celestia_getcategories);
    celx.registerMethod("getrootcategories", celestia_getrootcategories);
    celx.registerMethod("bindtranslationdomain", celestia_bindtranslationdomain);
    celx.registerMethod("setasterisms", celestia_setasterisms);
    celx.pop(1);
}
