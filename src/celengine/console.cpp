// console.cpp
//
// Copyright (C) 2003, Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include <cstring>
#include <cstdarg>
#include <cstdio>
#include <cassert>
#include <algorithm>
#include "celutil/utf8.h"
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
    text = new wchar_t[(nColumns + 1) * nRows];
    for (int i = 0; i < nRows; i++)
        text[(nColumns + 1) * i] = '\0';
}


Console::~Console()
{
    if (text != NULL)
        delete[] text;
}


/*! Resize the console log to use the specified number of rows.
 *  Old long entries are preserved in the resize. setRowCount()
 *  returns true if it was able to successfully allocate a new
 *  buffer, and false if there was a problem (out of memory.)
 */
bool Console::setRowCount(int _nRows)
{
    wchar_t* newText = new wchar_t[(nColumns + 1) * _nRows];
    if (newText == NULL)
        return false;

    for (int i = 0; i < _nRows; i++)
    {
        newText[(nColumns + 1) * i] = '\0';
    }

    std::copy(newText, newText + (nColumns + 1) * min(_nRows, nRows), text);

    delete[] text;
    text = newText;
    nRows = _nRows;

    return true;
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
            wchar_t ch = text[r * (nColumns + 1) + j];
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

void Console::print(wchar_t c)
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
    int length = strlen(s);
    bool validChar = true;
    int i = 0;

    while (i < length && validChar)
    {
        wchar_t ch = 0;
        validChar = UTF8Decode(s, i, length, ch);
        i += UTF8EncodedSize(ch);
        print(ch);
    }
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
    {
        switch (decodeState)
        {
        case UTF8DecodeStart:
            if (c < 0x80)
            {
                // Just a normal 7-bit character
                console->print((char) c);
            }
            else
            {
                unsigned int count;

                if ((c & 0xe0) == 0xc0)
                    count = 2;
                else if ((c & 0xf0) == 0xe0)
                    count = 3;
                else if ((c & 0xf8) == 0xf0)
                    count = 4;
                else if ((c & 0xfc) == 0xf8)
                    count = 5;
                else if ((c & 0xfe) == 0xfc)
                    count = 6;
                else
                    count = 1; // Invalid byte

                if (count > 1)
                {
                    unsigned int mask = (1 << (7 - count)) - 1;
                    decodeShift = (count - 1) * 6;
                    decodedChar = (c & mask) << decodeShift;
                    decodeState = UTF8DecodeMultibyte;
                }
                else
                {
                    // If the character isn't valid multibyte sequence head,
                    // silently skip it by leaving the decoder state alone.
                }
            }
            break;

        case UTF8DecodeMultibyte:
            if ((c & 0xc0) == 0x80)
            {
                // We have a valid non-head byte in the sequence
                decodeShift -= 6;
                decodedChar |= (c & 0x3f) << decodeShift;
                if (decodeShift == 0)
                {
                    console->print(decodedChar);
                    decodeState = UTF8DecodeStart;
                }
            }
            else
            {
                // Bad byte in UTF-8 encoded sequence; we'll silently ignore
                // it and reset the state of the UTF-8 decoder.
                decodeState = UTF8DecodeStart;
            }
            break;
        }
    }

    return c;
}
