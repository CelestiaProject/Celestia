// console.h
// 
// Copyright (C) 2001, Chris Laurel <claurel@shatters.net>
//
// Text console class for OpenGL.  The console supports both printf
// and C++ operator style mechanisms for output.
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#ifndef _CONSOLE_H_
#define _CONSOLE_H_

#include <string>
#include <iostream>
#include "gl.h"
#include "texfont.h"

using namespace std;


class Console;

// Custom streambuf class to support C++ operator style output.  The
// output is completely unbuffered so that it can coexist with printf
// style output which the Console class also supports.
class ConsoleStreamBuf : public std::streambuf
{
 public:
    ConsoleStreamBuf() : console(NULL) { setbuf(0, 0); };

    setConsole(Console*);

    int overflow(int c = EOF);

 private:
    Console* console;
};


class Console : public std::ostream
{
 public:
    Console(int rows, int cols);
    ~Console();

    void setFont(TexFont*);
    TexFont* getFont();

    void setOrigin(float, float);
    void setScale(float, float);

    void render();
    void clear();
    void home();
    void scrollUp();
    void scrollDown();
    void cursorRight();
    void cursorLeft();
    void CR();
    void LF();
    void backspace();

    inline void setChar(char c);
    void setCharAt(char c, int row, int col);

    void print(char);
    void print(char*);
    void printf(char*, ...);

    //    Console& operator<<(string);

 private:
    int nRows;
    int nColumns;
    char** text;

    TexFont* font;
    
    int cursorRow;
    int cursorColumn;

    float xscale, yscale;
    float left, top;

    ConsoleStreamBuf sbuf;
};


void Console::setChar(char c)
{
    text[cursorRow][cursorColumn] = c;
}


#endif // _CONSOLE_H_
