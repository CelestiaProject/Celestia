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
#include <Eigen/Core>

class Renderer;

struct TextureFontPrivate;
class TextureFont
{
    TextureFont(const Renderer*);
 public:
    TextureFont() = delete;
    ~TextureFont();
    TextureFont(const TextureFont&) = delete;
    TextureFont(TextureFont&&) = delete;
    TextureFont& operator=(const TextureFont&) = delete;
    TextureFont& operator=(TextureFont&&) = delete;

    void setMVPMatrices(const Eigen::Matrix4f& p, const Eigen::Matrix4f& m = Eigen::Matrix4f::Identity());

    float render(wchar_t c, float xoffset = 0.0f, float yoffset = 0.0f) const;
    float render(const std::string& str, float xoffset = 0.0f, float yoffset = 0.0f) const;

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
    void unbind();
    bool buildTexture();
    void flush();

    static TextureFont* load(const Renderer*, const fs::path&, int index, int size, int dpi);

 private:
    TextureFontPrivate *impl;
};

TextureFont* LoadTextureFont(const Renderer*, const fs::path&, int index = 0, int size = 0, int dpi = 96);
