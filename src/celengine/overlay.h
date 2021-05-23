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

#include <iosfwd>
#include <string>
#include <vector>
#include <Eigen/Core>

class Color;
class Overlay;
class Renderer;
class TextureFont;

namespace celestia
{
class Rect;
}

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
    EIGEN_MAKE_ALIGNED_OPERATOR_NEW

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
    void print(wchar_t);
    void print(char);
    void print(const char*);

 private:
    int windowWidth{ 1 };
    int windowHeight{ 1 };
    std::shared_ptr<TextureFont> font{ nullptr };
    bool useTexture{ false };
    bool fontChanged{ false };
    int textBlock{ 0 };

    float xoffset{ 0.0f };
    float yoffset{ 0.0f };

    OverlayStreamBuf sbuf;

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

#endif // _OVERLAY_H_
