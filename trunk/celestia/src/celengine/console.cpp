// console.cpp
//
// Copyright (C) 2003, Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include <cstdarg>
#include <cstdio>
#include <cassert>
#include "gl.h"
#include "vecgl.h"
#include "console.h"

using namespace std;


static int pmod(int n, int m)
{
    return n >= 0 ? n % m : m - (-(n + 1) % m) - 1;
}


Console::Console(int _nRows, int _nColumns) :
    ostream(&sbuf),
    text(NULL),
    nRows(_nRows),
    nColumns(_nColumns),
    row(0),
    column(0),
    windowRow(0),
    windowHeight(10),
    xscale(1),
    yscale(1),
    font(NULL),
    autoScroll(true)
{
    sbuf.setConsole(this);
    text = new char[(nColumns + 1) * nRows];
    for (int i = 0; i < nRows; i++)
        text[(nColumns + 1) * i] = '\0';
}


Console::~Console()
{
    if (text != NULL)
        delete[] text;
}


void Console::begin()
{
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    gluOrtho2D(0, xscale, 0, yscale);
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();
    glTranslatef(0.125f, 0.125f, 0);

    glDisable(GL_LIGHTING);
    glDisable(GL_TEXTURE_2D);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}


void Console::end()
{
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();
}


void Console::render(int rowHeight)
{
    if (font == NULL)
        return;

    glEnable(GL_TEXTURE_2D);
    font->bind();
    glPushMatrix();
    for (int i = 0; i < rowHeight; i++)
    {
        //int r = (nRows - rowHeight + 1 + windowRow + i) % nRows;
        int r = pmod(row + windowRow + i, nRows);
        for (int j = 0; j < nColumns; j++)
        {
            char ch = text[r * (nColumns + 1) + j];
            if (ch == '\0')
                break;
            font->render(ch);
        }

        // advance to the next line
        glPopMatrix();
        glTranslatef(0.0f, -(1.0f + font->getHeight()), 0.0f);
        glPushMatrix();
    }
    glPopMatrix();
}


void Console::setScale(int w, int h)
{
    xscale = w;
    yscale = h;
}


void Console::setFont(TextureFont* f)
{
    if (f != font)
    {
        font = f;
    }
}


void Console::newline()
{
    assert(column <= nColumns);
    assert(row < nRows);

    text[row * (nColumns + 1) + column] = '\0';
    row = (row + 1) % nRows;
    column = 0;

    if (autoScroll)
        windowRow = -windowHeight;
}


void Console::print(char c)
{
    switch (c)
    {
    case '\n':
        newline();
        break;
    default:
        if (column == nColumns)
            newline();
        text[row * (nColumns + 1) + column] = c;
        column++;
        break;
    }
}


void Console::print(char* s)
{
    while (*s != '\0')
        print(*s++);
}


#if 0
void Console::printf(const char* format, ...)
{
    va_list args;
    va_start(args, format);

    char buf[1024];
    vsprintf(buf, format, args);
    print(buf);
    
    va_end(args);
}
#endif


int Console::getRow() const
{
    return row;
}


int Console::getColumn() const
{
    return column;
}


int Console::getWindowRow() const
{
    return windowRow;
}


void Console::setWindowRow(int _row)
{
    windowRow = _row;
}


void Console::setWindowHeight(int _height)
{
    windowHeight = _height;
}


int Console::getWidth() const
{
    return nColumns;
}


int Console::getHeight() const
{
    return nRows;
}


//
// ConsoleStreamBuf implementation
//
void ConsoleStreamBuf::setConsole(Console* c)
{
    console = c;
}

int ConsoleStreamBuf::overflow(int c)
{
    if (console != NULL)
        console->print((char) c);
    return c;
}
