// texturefont.cpp
//
// Copyright (C) 2001, Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include "texturefont.h"

using namespace std;


TextureFont::TextureFont(TexFont* _txf) : txf(_txf)
{
    txfEstablishTexture(txf, 0, GL_FALSE);
}

TextureFont::~TextureFont()
{
    if (txf != NULL)
        txfUnloadFont(txf);
}

void TextureFont::render(int c)
{
    txfRenderGlyph(txf, c);
}

void TextureFont::render(string s)
{
    txfRenderString(txf, s);
}

int TextureFont::getWidth(string s)
{
    int width = 0;
    int maxAscent = 0;
    int maxDescent = 0;
    txfGetStringMetrics(txf, const_cast<char*> (s.c_str()), s.size(),
                        width, maxAscent, maxDescent);

    return width;
}

int TextureFont::getHeight()
{
    return txf->max_ascent + txf->max_descent;
}

int TextureFont::getTextureName()
{
    return txf->texobj;
}

void TextureFont::bind()
{
    glBindTexture(GL_TEXTURE_2D, getTextureName());
}
    
TextureFont* LoadTextureFont(string filename)
{
    TexFont* txf = txfLoadFont(const_cast<char*> (filename.c_str()));
    if (txf == NULL)
        return NULL;
    else
        return new TextureFont(txf);
}
