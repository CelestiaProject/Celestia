// celx_frame.h
//
// Copyright (C) 2003-2008, the Celestia Development Team
//
// Lua script extensions for Celestia: frame object
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#ifndef _CELX_FRAME_H_
#define _CELX_FRAME_H_

struct lua_State;
class ObserverFrame;

extern void CreateFrameMetaTable(lua_State* l);
extern int frame_new(lua_State* l, const ObserverFrame& frame);
extern ObserverFrame* to_frame(lua_State* l, int index);

#endif // _CELX_FRAME_H_
