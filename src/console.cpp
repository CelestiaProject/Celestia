// console.cpp
// 
// Copyright (C) 2000, Chris Laurel <claurel@shatters.net>
//
// Text console class for OpenGL

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <cstdarg>
#include "gl.h"
#include "texfont.h"
#include "console.h"

using namespace std;


Console::Console(int rows, int cols) :
    ostream(&sbuf),
    nRows(rows),
    nColumns(cols),
    cursorRow(0),
    cursorColumn(0),
    font(NULL),
    xscale(1.0f),
    yscale(1.0f)
{
    text = new char*[nRows];
    for (int i = 0; i < nRows; i++)
        text[i] = new char[nColumns];
    clear();
    sbuf.setConsole(this);
}


Console::~Console()
{
    if (text != NULL)
    {
        for (int i = 0; i < nRows; i++)
        {
            if (text[i] != NULL)
                delete[] text[i];
        }
        delete[] text;
    }
}


void Console::setFont(TexFont* _font)
{
    font = _font;
}


TexFont* Console::getFont()
{
    return font;
}


void Console::setScale(float _xscale, float _yscale)
{
    xscale = _xscale;
    yscale = _yscale;
}


void Console::clear()
{
    for (int i = 0; i < nRows; i++)
        memset(text[i], '\0', nColumns);
}


void Console::home()
{
    cursorColumn = 0;
    cursorRow = 0;
}


void Console::scrollUp()
{
    for (int i = 1; i < nRows; i++)
    {
        strcpy(text[i - 1], text[i]);
    }
    memset(text[nRows - 1], '\0', nColumns);
}


void Console::scrollDown()
{
    for (int i = nRows - 1; i > 0; i++)
    {
        strcpy(text[i], text[i - 1]);
    }
    memset(text[0], '\0', nColumns);
}


void Console::cursorRight()
{
    if (cursorColumn < nColumns - 1)
        cursorColumn++;
}


void Console::cursorLeft()
{
    if (cursorColumn > 0)
        cursorColumn--;
}


void Console::CR()
{
    cursorColumn = 0;
}


void Console::LF()
{
    cursorRow++;
    if (cursorRow == nRows)
    {
        cursorRow--;
        scrollUp();
    }
}


void Console::setCharAt(char c, int row, int col)
{
    if (row >= 0 && row < nRows && col >= 0 && col < nColumns)
        text[row][col] = c;
}


void Console::print(char c)
{
    switch (c)
    {
    case '\n':
        CR();
        LF();
        break;
    case '\r':
        CR();
        break;
    case '\b':
        cursorLeft();
        break;
    default:
        text[cursorRow][cursorColumn] = c;
        cursorRight();
        break;
    }
}


void Console::print(char* s)
{
    while (*s != '\0')
        print(*s++);
}


void Console::printf(char* format, ...)
{
    va_list args;
    va_start(args, format);

    char buf[1024];
    vsprintf(buf, format, args);
    print(buf);
    
    va_end(args);
}


void Console::render()
{
    float width = 1.8f;
    float height = 1.8f;
    float top = 0.9f;
    float left = -0.9f;
    float aspectRatio = 4.0f / 3.0f;
    float charHeight = height / (float) nRows;
    float charWidth = charHeight / aspectRatio / 2.0f;
    float scale = 0.005f;

    if (font == NULL)
        return;

    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, font->texobj);

    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();
    glTranslatef(-1, 0.9f, -1);
    glScalef(xscale, yscale, 1);

    for (int i = 0; i < nRows; i++)
    {
        float x = left;
        char* s = text[i];

        glPushMatrix();
        while (*s != '\0')
        {
            txfRenderGlyph(font, *s);
            s++;
        }
        glPopMatrix();
        glTranslatef(0, -(1 + font->max_ascent + font->max_descent), 0);
    }

    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
}


ConsoleStreamBuf::setConsole(Console* c)
{
    console = c;
}

int ConsoleStreamBuf::overflow(int c)
{
    if (console != NULL)
        console->print((char) c);
    return c;
}
