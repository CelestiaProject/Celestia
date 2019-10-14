#pragma once

#include "celx_internal.h"

class TextureFont;
class Texture;

int celscript_from_string(lua_State* l, std::string& script_text);
void CreateCelscriptMetaTable(lua_State* l);

inline int celxClassId(TextureFont *)
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
