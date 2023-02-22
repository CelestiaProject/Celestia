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

#include <string>
#include <vector>
#include <fmt/printf.h>
#include <Eigen/Core>

class Color;
class Overlay;
class Renderer;
class TextureFont;

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

    void setColor(float r, float g, float b, float a);
    void setColor(const Color& c);

    void moveBy(float dx, float dy);
    void savePos();
    void restorePos();

    Renderer& getRenderer() const
    {
        return renderer;
    };

    void drawRectangle(const celestia::Rect&);

    void beginText();
    void endText();
    template <typename... T>
    void print(std::string_view format, const T&... args)
    {
        print_impl(fmt::format(format, args...));
    }
    template <typename... T>
    void printf(std::string_view format, const T&... args)
    {
        print_impl(fmt::sprintf(format, args...));
    }

 private:
    void print_impl(const std::string&);

    int windowWidth{ 1 };
    int windowHeight{ 1 };
    std::shared_ptr<TextureFont> font{ nullptr };
    bool useTexture{ false };
    bool fontChanged{ false };
    int textBlock{ 0 };

    float xoffset{ 0.0f };
    float yoffset{ 0.0f };

    Renderer& renderer;

    struct CursorPosition
    {
        void reset()
        {
            x = y = 0.125f;
        }
        float x, y;
    };
    CursorPosition global { 0.0f, 0.0f };
    std::vector<CursorPosition> posStack;
    Eigen::Matrix4f projection;
};
