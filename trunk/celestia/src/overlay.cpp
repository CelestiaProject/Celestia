// overlay.cpp
//
// Copyright (C) 2001, Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include <cstdarg>
#include <cstdio>
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
    glTranslatef(0.375f, 0.375f, 0);

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
    font = f;
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


void Overlay::print(char c)
{
    if (font != NULL)
    {
        if (!useTexture)
        {
            glEnable(GL_TEXTURE_2D);
            glBindTexture(GL_TEXTURE_2D, font->getTextureName());
            useTexture = true;
        }

        switch (c)
        {
        case '\n':
            if (textBlock > 0)
            {
                glPopMatrix();
                glTranslatef(0, -(1 + font->getHeight()), 0);
                glPushMatrix();
            }
            break;
        default:
            font->render(c);
            break;
        }
    }
}

void Overlay::print(char* s)
{
    while (*s != '\0')
        print(*s++);
}


void Overlay::printf(char* format, ...)
{
    va_list args;
    va_start(args, format);

    char buf[1024];
    vsprintf(buf, format, args);
    print(buf);
    
    va_end(args);
}


void Overlay::rect(float x, float y, float w, float h)
{
    if (useTexture)
    {
        glDisable(GL_TEXTURE_2D);
        useTexture = false;
    }
    glRectf(x, y, x + w, y + h);
}


//
// OverlayStreamBuf implementation
//
void OverlayStreamBuf::setOverlay(Overlay* o)
{
    overlay = o;
}

int OverlayStreamBuf::overflow(int c)
{
    if (overlay != NULL)
        overlay->print((char) c);
    return c;
}
