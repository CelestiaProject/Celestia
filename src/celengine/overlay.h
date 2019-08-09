// overlay.h
//
// Copyright (C) 2001, Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#ifndef _OVERLAY_H_
#define _OVERLAY_H_

#include <string>
#include <iostream>
#include <vector>
#include <celtxf/texturefont.h>
#include <celutil/color.h>


class Overlay;
class Renderer;

// Custom streambuf class to support C++ operator style output.  The
// output is completely unbuffered so that it can coexist with printf
// style output which the Overlay class also supports.
class OverlayStreamBuf : public std::streambuf
{
 public:
    OverlayStreamBuf();

    void setOverlay(Overlay*);

    int overflow(int c = EOF);

    enum UTF8DecodeState
    {
        UTF8DecodeStart     = 0,
        UTF8DecodeMultibyte = 1,
    };

 private:
    Overlay* overlay{ nullptr };

    UTF8DecodeState decodeState{ UTF8DecodeStart };
    wchar_t decodedChar{ 0 };
    unsigned int decodeShift{ 0 };
};


class Overlay : public std::ostream
{
 public:
    Overlay(const Renderer&);
    Overlay() = delete;
    ~Overlay() = default;

    void begin();
    void end();

    void setWindowSize(int, int);
    void setFont(TextureFont*);

    enum class RectType
    {
        Outlined    = 0x0001,
        Filled      = 0x0002,
        Textured    = 0x0004
    };

    struct Rectangle
    {
        Rectangle() = default;
        Rectangle(float _x, float _y, float _w, float _h, const Color& _c, RectType _t, float _lw = 1.0f) :
            x(_x), y(_y), w(_w), h(_h), type(_t), lw(_lw)
        {
            colors.push_back(_c);
        };
        Rectangle(float _x, float _y, float _w, float _h, const std::vector<Color>& _c, RectType _t, float _lw = 1.0f) :
            x(_x), y(_y), w(_w), h(_h), colors(_c), type(_t), lw(_lw)
        {
        };
        float x, y, w, h;
        float lw      { 1.0f };
        RectType type { RectType::Filled };
        std::vector<Color> colors;
    };

    void rect(const Rectangle&);

    void beginText();
    void endText();
    void print(wchar_t);
    void print(char);
    void print(const char*);

 private:
    int windowWidth{ 1 };
    int windowHeight{ 1 };
    TextureFont* font{ nullptr };
    bool useTexture{ false };
    bool fontChanged{ false };
    int textBlock{ 0 };

    float xoffset{ 0.0f };
    float yoffset{ 0.0f };

    float lineWidth { 1.0f };

    OverlayStreamBuf sbuf;

    const Renderer& renderer;
};

#endif // _OVERLAY_H_
