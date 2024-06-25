// overlay.h
//
// Copyright (C) 2001-present, the Celestia Development Team
// Original version by Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#pragma once

#include <locale>
#include <string>
#include <vector>
#include <fmt/printf.h>
#include <Eigen/Core>
#include <celengine/textlayout.h>

class Color;
class Overlay;
class Renderer;

namespace celestia
{
class Rect;
}

class Overlay
{
 public:
    Overlay(Renderer&);
    Overlay() = delete;
    ~Overlay() = default;

    void begin();
    void end();

    void setWindowSize(int, int);
    void setFont(const std::shared_ptr<TextureFont>&);
    void setTextAlignment(celestia::engine::TextLayout::HorizontalAlignment halign);

    void setColor(float r, float g, float b, float a);
    void setColor(const Color& c);
    void setColor(const Color& c, float a);

    void moveBy(float dx, float dy);
    void moveBy(int dx, int dy);
    void savePos();
    void restorePos();

    Renderer& getRenderer() const
    {
        return renderer;
    };

    void drawRectangle(const celestia::Rect&) const;

    void beginText();
    void endText();

    void print(std::string_view);

    template <typename... T>
    void print(const std::locale& loc, std::string_view format, const T&... args)
    {
        static_assert(sizeof...(args) > 0);
        print(fmt::format(loc, format, args...));
    }

    template <typename... T>
    void print(std::string_view format, const T&... args)
    {
        static_assert(sizeof...(args) > 0);
        print(fmt::format(format, args...));
    }

    template <typename... T>
    void printf(std::string_view format, const T&... args)
    {
        static_assert(sizeof...(args) > 0);
        print(fmt::sprintf(format, args...));
    }

 private:
    int windowWidth{ 1 };
    int windowHeight{ 1 };

    std::unique_ptr<celestia::engine::TextLayout> layout{ nullptr };

    Renderer& renderer;

    std::vector<std::pair<float, float>> posStack;
    Eigen::Matrix4f projection;
};
