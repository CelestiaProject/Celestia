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
#include <iostream>
#include <celtxf/texturefont.h>


class Console;

// Custom streambuf class to support C++ operator style output.  The
// output is completely unbuffered.
class ConsoleStreamBuf : public std::streambuf
{
 public:
    ConsoleStreamBuf() : console(NULL) { setbuf(0, 0); };

    void setConsole(Console*);

    int overflow(int c = EOF);
    enum UTF8DecodeState
    {
        UTF8DecodeStart     = 0,
        UTF8DecodeMultibyte = 1,
    };

 private:
    Console* console;
    UTF8DecodeState decodeState;
    wchar_t decodedChar;
    unsigned int decodeShift;
};


class Console : public std::ostream
{
 public:
    Console(int _nRows, int _nColumns);
    ~Console();

    bool setRowCount(int _nRows);

    void begin();
    void end();
    void render(int rowHeight);

    void setScale(int, int);
    void setFont(TextureFont*);

    void print(wchar_t);
    void print(char*);
    void newline();
#if 0
    void printf(const char*, ...);
#endif    

    int getRow() const;
    int getColumn() const;
    int getWindowRow() const;
    void setWindowRow(int);
    void setWindowHeight(int);

    int getHeight() const;
    int getWidth() const;

 private:
    wchar_t* text;
    int nRows;
    int nColumns;
    int row;
    int column;

    int windowRow;

    int windowHeight;

    int xscale;
    int yscale;
    TextureFont* font;

    ConsoleStreamBuf sbuf;

    bool autoScroll;
};

#endif // _CELENGINE_CONSOLE_H_
