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
#include "celx.h"
#include "celestiacore.h"
extern "C" {
#include "lua/lualib.h"
}

using namespace std;

static char* ClassNames[] =
{
    "class_celestia",
    "class_observer",
    "class_object",
    "class_vec3",
    "class_matrix",
    "class_rotation"
};

static const int _Celestia = 0;
static const int _Observer = 1;
static const int _Object   = 2;
static const int _Vec3     = 3;
static const int _Matrix   = 4;
static const int _Rotation = 5;

#define CLASS(i) ClassNames[(i)]

static void lua_pushclass(lua_State* l, int id)
{
    lua_pushlstring(l, ClassNames[id], sizeof(ClassNames[id]) - 1);
}

static void lua_setclass(lua_State* l, int id)
{
    lua_pushclass(l, id);
    lua_rawget(l, LUA_REGISTRYINDEX);
    lua_setmetatable(l, -2);
}


LuaState::LuaState()
{
    state = lua_open();
}

LuaState::~LuaState()
{
    if (state != NULL)
        lua_close(state);
}

lua_State* LuaState::getState() const
{
    return state;
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

int LuaState::loadScript(istream& in)
{
    char buf[4096];
    ReadChunkInfo info;
    info.buf = buf;
    info.bufSize = sizeof(buf);
    info.in = &in;

    int status = lua_load(state, readStreamChunk, &info, "stream");
    
    return status;
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
    else
        return 0;
}


static int celestia_new(lua_State* l, CelestiaCore* appCore)
{
    CelestiaCore** ud = reinterpret_cast<CelestiaCore**>(lua_newuserdata(l, sizeof(CelestiaCore*)));
    *ud = appCore;

    lua_setclass(l, _Celestia);

    return 1;
}


static CelestiaCore* to_celestia(lua_State* l, int index)
{
    CelestiaCore** c = reinterpret_cast<CelestiaCore**>(lua_touserdata(l, index));
    if (c == NULL)
        return NULL;
    else
        return *c;
}


static int celestia_flash(lua_State* l)
{
    int argc = lua_gettop(l);
    if (argc != 2)
    {
        lua_pushstring(l, "One argument expected to function celestia:flash");
        lua_error(l);
    }

    CelestiaCore* appCore = to_celestia(l, 1);
    const char* s = lua_tostring(l, 2);

    if (appCore != NULL && s != NULL)
        appCore->flash(s, 5.0);

    return 0;
}


static int celestia_show(lua_State* l)
{
    int argc = lua_gettop(l);
    if (argc < 1)
    {
        lua_pushstring(l, "One argument expected to function celestia:show");
        lua_error(l);
    }

    CelestiaCore* appCore = to_celestia(l, 1);

    int flags = 0;
    for (int i = 2; i <= argc; i++)
    {
        const char* s = lua_tostring(l, i);
        if (s != NULL)
            flags |= parseRenderFlag(s);
    }

    if (appCore != NULL)
    {
        Renderer* r = appCore->getRenderer();
        r->setRenderFlags(r->getRenderFlags() | flags);
    }

    return 0;
}


static int celestia_hide(lua_State* l)
{
    int argc = lua_gettop(l);
    if (argc < 1)
    {
        lua_pushstring(l, "One argument expected to function celestia:hide");
        lua_error(l);
    }

    CelestiaCore* appCore = to_celestia(l, 1);

    int flags = 0;
    for (int i = 2; i <= argc; i++)
    {
        const char* s = lua_tostring(l, i);
        if (s != NULL)
            flags |= parseRenderFlag(s);
    }

    if (appCore != NULL)
    {
        Renderer* r = appCore->getRenderer();
        r->setRenderFlags(r->getRenderFlags() & ~flags);
    }

    return 0;
}


static int celestia_tostring(lua_State* l)
{
    lua_pushstring(l, "[Celestia]");

    return 1;
}


static void CreateCelestiaMetaTable(lua_State* l)
{
    lua_pushclass(l, _Celestia);
    lua_newtable(l);

    lua_pushliteral(l, "__tostring");
    lua_pushvalue(l, -2);
    lua_pushcclosure(l, celestia_tostring, 1);
    lua_rawset(l, -3);

    lua_pushliteral(l, "__index");
    lua_pushvalue(l, -2);
    lua_rawset(l, -3);
    lua_pushvalue(l, -1);

    lua_pushstring(l, "flash");
    lua_pushvalue(l, -2);
    lua_pushcclosure(l, celestia_flash, 1);
    lua_settable(l, -4);
    lua_pushstring(l, "show");
    lua_pushvalue(l, -2);
    lua_pushcclosure(l, celestia_show, 1);
    lua_settable(l, -4);
    lua_pushstring(l, "hide");
    lua_pushvalue(l, -2);
    lua_pushcclosure(l, celestia_hide, 1);
    lua_settable(l, -4);
    lua_pop(l, 1);

    lua_rawset(l, LUA_REGISTRYINDEX);
}


bool LuaState::init(CelestiaCore* appCore)
{
    CreateCelestiaMetaTable(state);

    // Create the celestia object
    lua_pushstring(state, "celestia");
    celestia_new(state, appCore);
    lua_settable(state, LUA_GLOBALSINDEX);

    return true;
}
