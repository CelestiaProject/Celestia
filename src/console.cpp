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
#include "console.h"

using namespace std;


Console::Console(int rows, int cols) :
    ostream(&sbuf),
    nRows(rows),
    nColumns(cols),
    cursorRow(0),
    cursorColumn(0),
    font(NULL)
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


void Console::setFont(TextureFont* _font)
{
    font = _font;
}


TextureFont* Console::getFont()
{
    return font;
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


void Console::backspace()
{
    cursorLeft();
    setCharAt('\0', cursorRow, cursorColumn);
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
    if (font == NULL)
        return;

    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, font->getTextureName());

    glPushMatrix();
    for (int i = 0; i < nRows; i++)
    {
        char* s = text[i];

        glPushMatrix();
        while (*s != '\0')
        {
            font->render((int) *s);
            s++;
        }
        glPopMatrix();
        glTranslatef(0, -(1 + font->getHeight()), 0);
    }
    glPopMatrix();
}


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
