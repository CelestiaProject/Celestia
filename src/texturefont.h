// texturefont.h
//
// Copyright (C) 2001, Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#ifndef _TEXTUREFONT_H_
#define _TEXTUREFONT_H_

#include <string>
#include "gl.h"
#include "texfont.h"


class TextureFont
{
 public:
    TextureFont(TexFont*);
    ~TextureFont();

    void render(int c);
    void render(std::string);

    int getWidth(std::string);
    //    int getWidth(int c);

    int getHeight();

    int getTextureName();

    void bind();

 private:
    TexFont* txf;
};

TextureFont* LoadTextureFont(std::string filename);


#endif // _TEXTUREFONT_H_

