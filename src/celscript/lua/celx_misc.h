// celx_misc.h
//
// Copyright (C) 2019, the Celestia Development Team
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#pragma once

#include "celx_internal.h"

class TextureFont;
class Texture;

int celscript_from_string(lua_State* l, std::string& script_text);
void CreateCelscriptMetaTable(lua_State* l);

inline int celxClassId(const std::shared_ptr<TextureFont>&)
{
    return Celx_Font;
}

void CreateFontMetaTable(lua_State*);

void CreateImageMetaTable(lua_State*);

inline int celxClassId(Texture *)
{
    return Celx_Texture;
}

void CreateTextureMetaTable(lua_State*);
