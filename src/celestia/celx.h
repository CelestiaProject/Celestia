// celx.h
// 
// Copyright (C) 2003, Chris Laurel <claurel@shatters.net>
//
// Lua script extensions for Celestia
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#ifndef _CELESTIA_CELX_H_
#define _CELESTIA_CELX_H_

#include <iostream>
extern "C" {
#include "lua/lua.h"
}

class CelestiaCore;

class LuaState
{
public:
    LuaState();
    ~LuaState();

    lua_State* getState() const;

    int loadScript(std::istream&);
    bool init(CelestiaCore*);

private:
    lua_State* state;
};

#endif // _CELESTIA_CELX_H_
