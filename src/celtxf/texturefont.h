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

#include <vector>
#include <string>
#include <iostream>
#include <celutil/filesystem.h>


class TextureFont
{
 public:
    TextureFont() = default;
    ~TextureFont();

    void render(wchar_t ch) const;
    void render(const std::string& s) const;

    void render(wchar_t ch, float xoffset, float yoffset) const;
    void render(const std::string& s, float xoffset, float yoffset) const;

    int getWidth(const std::string&) const;
    int getWidth(int c) const;
    int getMaxWidth() const;
    int getHeight() const;

    int getMaxAscent() const;
    void setMaxAscent(int);
    int getMaxDescent() const;
    void setMaxDescent(int);

    short getAdvance(wchar_t c)
    {
        const Glyph* glyph = getGlyph(c);
        if (glyph == nullptr)
            glyph = getGlyph((wchar_t) '?');
        return glyph->advance;
    }

    int getTextureName() const;

    void bind();

    bool buildTexture();

 public:
    struct TexCoord
    {
        float u, v;
    };

    struct Glyph
    {
        Glyph(unsigned short _id) : __id(_id) {};

        unsigned short __id;
        unsigned short width;
        unsigned short height;
        short x;
        short xoff;
        short y;
        short yoff;
        short advance;
        TexCoord texCoords[4];
    };

    enum {
        TxfByte = 0,
        TxfBitmap = 1,
    };

 private:
    void addGlyph(const Glyph&);
    const TextureFont::Glyph* getGlyph(wchar_t) const;
    void rebuildGlyphLookupTable();

 private:
    int maxAscent{ 0 };
    int maxDescent{ 0 };
    int maxWidth{ 0 };

    int texWidth{ 0 };
    int texHeight{ 0 };
    unsigned char* fontImage{ nullptr };
    unsigned int texName{ 0 };

    std::vector<Glyph> glyphs;

    const Glyph** glyphLookup{ nullptr };
    unsigned int glyphLookupTableSize{ 0 };

 public:
    static TextureFont* load(std::istream& in);
};

TextureFont* LoadTextureFont(const fs::path&);

#endif // _TEXTUREFONT_H_

