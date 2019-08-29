// overlay.cpp
//
// Copyright (C) 2001, Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include <cstring>
#include <cstdarg>
#include <celutil/utf8.h>
#include <GL/glew.h>
#include <Eigen/Core>
#include <celutil/debug.h>
#include "vecgl.h"
#include "overlay.h"
#include "render.h"

using namespace std;
using namespace Eigen;


Overlay::Overlay(const Renderer& r) :
    ostream(&sbuf),
    renderer(r)
{
    sbuf.setOverlay(this);
}

void Overlay::begin()
{
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    gluOrtho2D(0, windowWidth, 0, windowHeight);
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();
    glTranslatef(0.125f, 0.125f, 0);

    glDisable(GL_LIGHTING);
    glDisable(GL_TEXTURE_2D);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    useTexture = false;
}

void Overlay::end()
{
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();
}


void Overlay::setWindowSize(int w, int h)
{
    windowWidth = w;
    windowHeight = h;
}

void Overlay::setFont(TextureFont* f)
{
    if (f != font)
    {
        font = f;
        fontChanged = true;
    }
}


void Overlay::beginText()
{
    glPushMatrix();
    textBlock++;
}

void Overlay::endText()
{
    if (textBlock > 0)
    {
        textBlock--;
        xoffset = 0.0f;
        glPopMatrix();
    }
}


void Overlay::print(wchar_t c)
{
    if (font != nullptr)
    {
        if (!useTexture || fontChanged)
        {
            glEnable(GL_TEXTURE_2D);
            font->bind();
            useTexture = true;
            fontChanged = false;
        }

        switch (c)
        {
        case '\n':
            if (textBlock > 0)
            {
                glPopMatrix();
                glTranslatef(0.0f, (float) -(1 + font->getHeight()), 0.0f);
                xoffset = 0.0f;
                glPushMatrix();
            }
            break;
        default:
            font->render(c, xoffset, yoffset);
            xoffset += font->getAdvance(c);
            break;
        }
    }
}


void Overlay::print(char c)
{
    if (font != nullptr)
    {
        if (!useTexture || fontChanged)
        {
            glEnable(GL_TEXTURE_2D);
            font->bind();
            useTexture = true;
            fontChanged = false;
        }

        switch (c)
        {
        case '\n':
            if (textBlock > 0)
            {
                glPopMatrix();
                glTranslatef(0.0f, (float) -(1 + font->getHeight()), 0.0f);
                xoffset = 0.0f;
                glPushMatrix();
            }
            break;
        default:
            font->render(c, xoffset, yoffset);
            xoffset += font->getAdvance(c);
            break;
        }
    }
}

void Overlay::print(const char* s)
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

static void drawRect(const Renderer& r,
                     const array<float, 8>& vertices,
                     const vector<Vector4f>& colors,
                     Overlay::RectType type,
                     float linewidth)
{
    uint32_t p = 0;
    switch (type)
    {
    case Overlay::RectType::Textured:
        p |= ShaderProperties::HasTexture;
    // [[ fallthrough ]]
    case Overlay::RectType::Outlined:
    case Overlay::RectType::Filled:
        switch (colors.size())
        {
        case 0:
            break;
        case 1:
            p |= ShaderProperties::UniformColor;
            break;
        case 4:
            p |= ShaderProperties::PerVertexColor;
            break;
        default:
            fmt::fprintf(cerr, "Incorrect number of colors: %i\n", colors.size());
            break;
        }
    default:
        break;
    }

    auto prog = r.getShaderManager().getShader(ShaderProperties(p));
    if (prog == nullptr)
        return;

    constexpr array<short, 8> texels = {0, 1,  1, 1,  1, 0,  0, 0};

    auto s = static_cast<GLsizeiptr>(memsize(vertices) + memsize(texels) + 4*4*sizeof(GLfloat));
    static celgl::VertexObject vo{ GL_ARRAY_BUFFER, s, GL_DYNAMIC_DRAW };
    vo.bindWritable();

    if (!vo.initialized())
    {
        vo.allocate();
        vo.setBufferData(texels.data(), memsize(vertices), memsize(texels));
        vo.setVertices(2, GL_FLOAT);
        vo.setTextureCoords(2, GL_SHORT, false, 0, memsize(vertices));
        vo.setColors(4, GL_FLOAT, false, 0, memsize(vertices) + memsize(texels));
    }

    vo.setBufferData(vertices.data(), 0, memsize(vertices));
    if (colors.size() == 4)
        vo.setBufferData(colors.data(), memsize(vertices) + memsize(texels), 4*4*sizeof(GLfloat));

    prog->use();
    if (type == Overlay::RectType::Textured)
        prog->samplerParam("tex") = 0;

    if (colors.size() == 1)
        prog->vec4Param("color") = colors[0];

    if (type != Overlay::RectType::Outlined)
    {
        vo.draw(GL_TRIANGLE_FAN, 4);
    }
    else
    {
        if (linewidth != 1.0f)
            glLineWidth(linewidth);
        vo.draw(GL_LINE_LOOP, 4);
        if (linewidth != 1.0f)
            glLineWidth(1.0f);
    }

    glUseProgram(0);
    vo.unbind();
}

void Overlay::rect(const Overlay::Rectangle& r)
{
    if (useTexture && r.type != Overlay::RectType::Textured)
    {
        glDisable(GL_TEXTURE_2D);
        useTexture = false;
    }

    vector<Vector4f> cv;
    for (const Color& c : r.colors)
        cv.push_back(c.toVector4());

    array<float, 8> coord = { r.x, r.y,  r.x+r.w, r.y, r.x+r.w, r.y+r.h, r.x, r.y+r.h };
    drawRect(renderer, coord, cv, r.type, r.lw);
}


//
// OverlayStreamBuf implementation
//
OverlayStreamBuf::OverlayStreamBuf()
{
    setbuf(nullptr, 0);
};


void OverlayStreamBuf::setOverlay(Overlay* o)
{
    overlay = o;
}


int OverlayStreamBuf::overflow(int c)
{
    if (overlay != nullptr)
    {
        switch (decodeState)
        {
        case UTF8DecodeStart:
            if (c < 0x80)
            {
                // Just a normal 7-bit character
                overlay->print((char) c);
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
                    overlay->print(decodedChar);
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
