// celx_rotation.h
//
// Copyright (C) 2003-2008, the Celestia Development Team
//
// Lua script extensions for Celestia: position object
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#ifndef _CELX_ROTATION_H_
#define _CELX_ROTATION_H_

#include <celmath/quaternion.h>

struct lua_State;

extern void CreateRotationMetaTable(lua_State* l);
extern int rotation_new(lua_State* l, const Quatd& qd);
extern Quatd* to_rotation(lua_State* l, int index);

#endif // _CELX_ROTATION_H_
