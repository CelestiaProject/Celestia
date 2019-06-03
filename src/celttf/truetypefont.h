// truetypefont.h
//
// Copyright (C) 2019, Celestia Development Team
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#pragma once

#include <string>
#include <celcompat/filesystem.h>

struct TextureFontPrivate;
class TextureFont
{
 public:
    TextureFont();
    ~TextureFont();
    TextureFont(const TextureFont&) = delete;
    TextureFont(TextureFont&&) = delete;
    TextureFont& operator=(const TextureFont&) = delete;
    TextureFont& operator=(TextureFont&&) = delete;

    void render(wchar_t c) const;
    void render(const std::string& str) const;

    void render(wchar_t c, float xoffset, float yoffset) const;
    void render(const std::string& str, float xoffset, float yoffset) const;

    int getWidth(const std::string&) const;
    int getWidth(int c) const;
    int getMaxWidth() const;
    int getHeight() const;

    int getMaxAscent() const;
    void setMaxAscent(int);
    int getMaxDescent() const;
    void setMaxDescent(int);

    short getAdvance(wchar_t c) const;

    int getTextureName() const;
    void bind();
    bool buildTexture();

    static TextureFont* load(const fs::path&, int size, int dpi);

 private:
    TextureFontPrivate *impl;
};

TextureFont* LoadTextureFont(const fs::path&, int size = 12, int dpi = 96);
