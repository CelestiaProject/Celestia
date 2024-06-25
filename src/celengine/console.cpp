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

namespace math = celestia::math;

namespace
{

constexpr int pmod(int n, int m)
{
    return n >= 0 ? n % m : m - (-(n + 1) % m) - 1;
}

}


Console::Console(Renderer& _renderer, int _nRows, int _nColumns) :
    std::ostream(&sbuf),
    nRows(_nRows),
    nColumns(_nColumns),
    renderer(_renderer)
{
    sbuf.setConsole(this);
    text.resize((nColumns + 1) * nRows, u'\0');
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

    text.resize((nColumns + 1) * _nRows, u'\0');
    nRows = _nRows;

    return true;
}


void Console::begin()
{
    projection = math::Ortho2D(0.0f, (float)xscale, 0.0f, (float)yscale);

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
        std::u16string_view line{text.data() + (r * (nColumns + 1)), static_cast<std::size_t>(nColumns)};
        if (auto endpos = line.find(u'\0'); endpos != std::u16string_view::npos)
            line = line.substr(0, endpos);

        font->render(line, global.x, global.y);

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

void Console::print(char16_t c)
{
    if (c == '\n')
    {
        newline();
    }
    else
    {
        if (column == nColumns)
            newline();
        text[row * (nColumns + 1) + column] = c;
        column++;
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
    if (traits_type::eq_int_type(c, traits_type::eof()))
        return traits_type::eof();

    // for now we don't implement non-BMP characters in the console
    if (auto result = validator.check(static_cast<unsigned char>(c));
        result >= 0 && result < 0x10000)
        console->print(static_cast<char16_t>(result));

    return traits_type::not_eof(c);
}
