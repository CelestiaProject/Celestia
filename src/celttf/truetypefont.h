// truetypefont.h
//
// Copyright (C) 2019-2022, Celestia Development Team
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#pragma once

#include <memory>
#include <string_view>

#include <Eigen/Core>

#include <celcompat/filesystem.h>


class Renderer;
class TextureFont;

std::shared_ptr<TextureFont>
LoadTextureFont(const Renderer *, const fs::path &, int index = 0, int size = 0);

fs::path
ParseFontName(const fs::path &, int &, int &);

struct TextureFontPrivate;
class TextureFont
{
public:
    constexpr static int kDefaultSize = 12;

    TextureFont(const Renderer *);
    TextureFont() = delete;
    ~TextureFont();
    TextureFont(const TextureFont &) = delete;
    TextureFont(TextureFont &&)      = default;
    TextureFont &operator=(const TextureFont &) = delete;
    TextureFont &operator=(TextureFont &&) = default;

    void setMVPMatrices(const Eigen::Matrix4f &p,
                        const Eigen::Matrix4f &m = Eigen::Matrix4f::Identity());

    std::pair<float, float> render(std::u16string_view line, float xoffset = 0.0f, float yoffset = 0.0f) const;

    int getWidth(std::u16string_view) const;
    int getMaxWidth() const;
    int getHeight() const;

    int  getMaxAscent() const;
    void setMaxAscent(int);
    int  getMaxDescent() const;
    void setMaxDescent(int);

    void bind();
    void unbind();
    void flush();

private:
    std::unique_ptr<TextureFontPrivate> impl;

    friend std::shared_ptr<TextureFont>
    LoadTextureFont(const Renderer*, const fs::path&, int, int);
};
