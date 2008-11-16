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
#include <cstdio>
#include <celutil/utf8.h>
#include "gl.h"
#include "vecgl.h"
#include "overlay.h"

using namespace std;


Overlay::Overlay() :
    ostream(&sbuf),
    windowWidth(1),
    windowHeight(1),
    font(NULL),
    useTexture(false),
    fontChanged(false),
    textBlock(0)
{
    sbuf.setOverlay(this);
}

Overlay::~Overlay()
{
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
        glPopMatrix();
    }
}


void Overlay::print(wchar_t c)
{
    if (font != NULL)
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
                glPushMatrix();
            }
            break;
        default:
            font->render(c);
            break;
        }
    }
}


void Overlay::print(char c)
{
    if (font != NULL)
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
                glPushMatrix();
            }
            break;
        default:
            font->render(c);
            break;
        }
    }
}

void Overlay::print(const char* s)
{
#if 0
    while (*s != '\0')
        print(*s++);
#else
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
#endif    
}


void Overlay::oprintf(const char* format, ...)
{
    va_list args;
    va_start(args, format);

    char buf[1024];
    vsprintf(buf, format, args);
    print(buf);
    
    va_end(args);
}


void Overlay::rect(float x, float y, float w, float h, bool fill)
{
    if (useTexture)
    {
        glDisable(GL_TEXTURE_2D);
        useTexture = false;
    }

    if (fill)
    {
        glRectf(x, y, x + w, y + h);
    }
    else
    {
        glBegin(GL_LINE_LOOP);
        glVertex3f(x, y, 0);
        glVertex3f(x + w, y, 0);
        glVertex3f(x + w, y + h, 0);
        glVertex3f(x, y + h, 0);
        glEnd();
    }
}


//
// OverlayStreamBuf implementation
//
OverlayStreamBuf::OverlayStreamBuf() :
    overlay(NULL),
    decodeState(UTF8DecodeStart)
{
    setbuf(0, 0);
};


void OverlayStreamBuf::setOverlay(Overlay* o)
{
    overlay = o;
}


int OverlayStreamBuf::overflow(int c)
{
    if (overlay != NULL)
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
