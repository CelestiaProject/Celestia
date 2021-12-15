// console.h
//
// Copyright (C) 2003, Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#ifndef _CELENGINE_CONSOLE_H_
#define _CELENGINE_CONSOLE_H_

#include <string>
#include <iosfwd>
#include <celutil/color.h>
#include <vector>
#include <Eigen/Core>

class Console;
class TextureFont;

// Custom streambuf class to support C++ operator style output.  The
// output is completely unbuffered.
class ConsoleStreamBuf : public std::streambuf
{
 public:
    ConsoleStreamBuf() { setbuf(0, 0); };

    void setConsole(Console*);

    int overflow(int c = EOF);
    enum UTF8DecodeState
    {
        UTF8DecodeStart     = 0,
        UTF8DecodeMultibyte = 1,
    };

 private:
    Console* console{ nullptr };
    UTF8DecodeState decodeState{ UTF8DecodeStart };
    wchar_t decodedChar{ 0 };
    unsigned int decodeShift{ 0 };
};

class Renderer;
class Console : public std::ostream
{
 public:
    static constexpr const int PageRows = 10;


    Console(Renderer& renderer, int _nRows, int _nColumns);
    ~Console();

    bool setRowCount(int _nRows);

    void begin();
    void end();
    void render(int rowHeight);

    void setScale(int, int);
    void setFont(const std::shared_ptr<TextureFont>&);

    void setColor(float r, float g, float b, float a) const;
    void setColor(const Color& c) const;

    void moveBy(float dx, float dy);

    void print(wchar_t);
    void print(char*);
    void newline();

    int getRow() const;
    int getColumn() const;
    int getWindowRow() const;
    void setWindowRow(int);
    void setWindowHeight(int);

    int getHeight() const;
    int getWidth() const;

    void scroll(int lines);

 private:
    void savePos();
    void restorePos();

    wchar_t* text{ nullptr };
    int nRows;
    int nColumns;
    int row{ 0 };
    int column{ 0 };

    int windowRow{ 0 };

    int windowHeight{ 10 };

    int xscale{ 1 };
    int yscale{ 1 };
    std::shared_ptr<TextureFont> font{ nullptr };
    Renderer& renderer;

    ConsoleStreamBuf sbuf;

    bool autoScroll{ true };

    float xoffset { 0.0f };
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

#endif // _CELENGINE_CONSOLE_H_
