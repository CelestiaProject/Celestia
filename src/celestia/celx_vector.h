// celx_vector.h
//
// Copyright (C) 2003-2008, the Celestia Development Team
//
// Lua script extensions for Celestia: vector object
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#ifndef _CELX_VECTOR_H_
#define _CELX_VECTOR_H_

struct lua_State;

extern void CreateVectorMetaTable(lua_State* l);
extern int vector_new(lua_State* l, const Vec3d& v);
extern Vec3d* to_vector(lua_State* l, int index);

#endif // _CELX_VECTOR_H_
