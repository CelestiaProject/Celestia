#pragma once

int celscript_from_string(lua_State* l, string& script_text);
void CreateCelscriptMetaTable(lua_State* l);

int font_new(lua_State*, TextureFont*);
void CreateFontMetaTable(lua_State*);

void CreateImageMetaTable(lua_State*);

int texture_new(lua_State*, Texture*);
void CreateTextureMetaTable(lua_State*);
