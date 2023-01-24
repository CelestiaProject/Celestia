// console.cpp
//
// Copyright (C) 2003, Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include "console.h"

#include <algorithm>
#include <cassert>

#include <celmath/geomutil.h>
#include <celttf/truetypefont.h>
#include <celutil/color.h>
#include "glsupport.h"
#include "render.h"
#include "shadermanager.h"

namespace
{

constexpr int pmod(int n, int m)
{
    return n >= 0 ? n % m : m - (-(n + 1) % m) - 1;
}

}


Console::Console(Renderer& _renderer, int _nRows, int _nColumns) :
    std::ostream(&sbuf),
    renderer(_renderer),
    nRows(_nRows),
    nColumns(_nColumns)
{
    sbuf.setConsole(this);
    text.resize((nColumns + 1) * nRows, L'\0');
}


/*! Resize the console log to use the specified number of rows.
 *  setRowCount() returns true if it was able to successfully allocate
 *  a new buffer, and false if there was a problem (out of memory.)
 *  This is currently only called on startup, so no attempt to ensure
 *  sensible preservation of old log entries is made.
 */
bool Console::setRowCount(int _nRows)
{
    if (_nRows == nRows)
        return true;

    text.resize((nColumns + 1) * _nRows, L'\0');
    nRows = _nRows;

    return true;
}


void Console::begin()
{
    projection = celmath::Ortho2D(0.0f, (float)xscale, 0.0f, (float)yscale);

    Renderer::PipelineState ps;
    ps.blending = true;
    ps.blendFunc = {GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA};
    ps.depthMask = true;
    renderer.setPipelineState(ps);

    global.reset();
}


void Console::end()
{
    if (font != nullptr)
        font->unbind();
}


void Console::render(int rowHeight)
{
    if (font == nullptr)
        return;

    font->bind();
    font->setMVPMatrices(projection);
    savePos();
    for (int i = 0; i < rowHeight; i++)
    {
        //int r = (nRows - rowHeight + 1 + windowRow + i) % nRows;
        int r = pmod(row + windowRow + i, nRows);
        for (int j = 0; j < nColumns; j++)
        {
            wchar_t ch = text[r * (nColumns + 1) + j];
            if (ch == '\0')
                break;
            font->render(ch, global.x + xoffset, global.y);
            xoffset += font->getAdvance(ch);
        }

        // advance to the next line
        restorePos();
        global.y -= 1.0f + font->getHeight();
        savePos();
    }
    restorePos();
}


void Console::setScale(int w, int h)
{
    xscale = w;
    yscale = h;
}


void Console::setFont(const std::shared_ptr<TextureFont>& f)
{
    if (f != font)
    {
        if (font != nullptr)
            font->flush();
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


void Console::setColor(float r, float g, float b, float a) const
{
    if (font != nullptr)
        font->flush();
    glVertexAttrib4f(CelestiaGLProgram::ColorAttributeIndex, r, g, b, a);
}


void Console::setColor(const Color& c) const
{
    if (font != nullptr)
        font->flush();
    glVertexAttrib4f(CelestiaGLProgram::ColorAttributeIndex,
                     c.red(), c.green(), c.blue(), c.alpha());

}


void Console::moveBy(float dx, float dy)
{
    global.x += dx;
    global.y += dy;
}


void Console::savePos()
{
    posStack.push_back(global);
}


void Console::restorePos()
{
    if (!posStack.empty())
    {
        global = posStack.back();
        posStack.pop_back();
    }
    xoffset = 0.0f;
}


void Console::scroll(int lines)
{
    int topRow = getWindowRow();
    int height = getHeight();

    if (lines < 0)
    {
        int scrollLimit = -std::min(height - 1, row);
        if (topRow + lines >= scrollLimit)
            setWindowRow(topRow + lines);
        else
            setWindowRow(scrollLimit);
    }
    else
    {
        if (topRow + lines <= -Console::PageRows)
            setWindowRow(topRow + lines);
        else
            setWindowRow(-Console::PageRows);
    }
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
    if (console != nullptr)
    {
        switch (decodeState)
        {
        case UTF8DecodeState::Start:
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
                    decodeState = UTF8DecodeState::Multibyte;
                }
                else
                {
                    // If the character isn't valid multibyte sequence head,
                    // silently skip it by leaving the decoder state alone.
                }
            }
            break;

        case UTF8DecodeState::Multibyte:
            if ((c & 0xc0) == 0x80)
            {
                // We have a valid non-head byte in the sequence
                decodeShift -= 6;
                decodedChar |= (c & 0x3f) << decodeShift;
                if (decodeShift == 0)
                {
                    console->print(decodedChar);
                    decodeState = UTF8DecodeState::Start;
                }
            }
            else
            {
                // Bad byte in UTF-8 encoded sequence; we'll silently ignore
                // it and reset the state of the UTF-8 decoder.
                decodeState = UTF8DecodeState::Start;
            }
            break;
        }
    }

    return c;
}
