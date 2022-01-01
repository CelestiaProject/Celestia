// truetypefont.h
//
// Copyright (C) 2019-2022, Celestia Development Team
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#pragma once

#include <Eigen/Core>
#include <celcompat/filesystem.h>
#include <string_view>

class Renderer;
class TextureFont;

std::shared_ptr<TextureFont>
LoadTextureFont(const Renderer *, const fs::path &, int index = 0, int size = 0);

struct TextureFontPrivate;
class TextureFont
{
 public:
    TextureFont(const Renderer *);
    TextureFont() = delete;
    ~TextureFont() = default;
    TextureFont(const TextureFont &) = delete;
    TextureFont(TextureFont &&)      = delete;
    TextureFont &operator=(const TextureFont &) = delete;
    TextureFont &operator=(TextureFont &&) = delete;

    void setMVPMatrices(const Eigen::Matrix4f &p,
                        const Eigen::Matrix4f &m = Eigen::Matrix4f::Identity());

    float render(wchar_t c, float xoffset = 0.0f, float yoffset = 0.0f) const;
    float render(std::string_view str, float xoffset = 0.0f, float yoffset = 0.0f) const;

    int getWidth(std::string_view) const;
    int getWidth(int c) const;
    int getMaxWidth() const;
    int getHeight() const;

    int  getMaxAscent() const;
    void setMaxAscent(int);
    int  getMaxDescent() const;
    void setMaxDescent(int);

    short getAdvance(wchar_t c) const;

    void bind();
    void unbind();
    void flush();

 private:
    std::unique_ptr<TextureFontPrivate> impl;

    friend std::shared_ptr<TextureFont>
    LoadTextureFont(const Renderer*, const fs::path&, int, int);
};
