// texturefont.cpp
//
// Copyright (C) 2001, Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include <cassert>
#include <cstring>
#include <fstream>

#ifndef _WIN32
#ifndef TARGET_OS_MAC
#include <config.h>
#endif /* TARGET_OS_MAC */
#endif /* _WIN32 */

#include <celutil/debug.h>
#include <celutil/bytes.h>
#include <celutil/utf8.h>
#include <celutil/util.h>
#include <celengine/gl.h>
#include "texturefont.h"

using namespace std;


TextureFont::TextureFont() :
    maxAscent(0),
    maxDescent(0),
    maxWidth(0),
    texWidth(0),
    texHeight(0),
    fontImage(NULL),
    texName(0),
    glyphLookup(NULL),
    glyphLookupTableSize(0)
{
}


TextureFont::~TextureFont()
{
    if (texName != 0)
        glDeleteTextures(1, (const GLuint*) &texName);
    if (fontImage != NULL)
        delete[] fontImage;
    if (glyphLookup != NULL)
        delete[] glyphLookup;
}


void TextureFont::render(wchar_t ch) const
{
    const Glyph* glyph = getGlyph(ch);
    if (glyph == NULL) glyph = getGlyph((wchar_t)'?');
    if (glyph != NULL)
    {
        glBegin(GL_QUADS);
        glTexCoord2f(glyph->texCoords[0].u, glyph->texCoords[0].v);
        glVertex2f(glyph->xoff, glyph->yoff);
        glTexCoord2f(glyph->texCoords[1].u, glyph->texCoords[1].v);
        glVertex2f(glyph->xoff + glyph->width, glyph->yoff);
        glTexCoord2f(glyph->texCoords[2].u, glyph->texCoords[2].v);
        glVertex2f(glyph->xoff + glyph->width, glyph->yoff + glyph->height);
        glTexCoord2f(glyph->texCoords[3].u, glyph->texCoords[3].v);
        glVertex2f(glyph->xoff, glyph->yoff + glyph->height);
        glEnd();
        glTranslatef(glyph->advance, 0.0f, 0.0f);
    }
}


void TextureFont::render(const string& s) const
{
    int len = s.length();
    bool validChar = true;
    int i = 0;
	
	while (i < len && validChar) {
        wchar_t ch = 0;
        validChar = UTF8Decode(s, i, ch);
        i += UTF8EncodedSize(ch);
        
        render(ch);
    }
}


int TextureFont::getWidth(const string& s) const
{
    int width = 0;
    int len = s.length();
    bool validChar = true;
	int i = 0;

    while (i < len && validChar)
    {
        wchar_t ch = 0;
        validChar = UTF8Decode(s, i, ch);
        i += UTF8EncodedSize(ch);

        const Glyph* g = getGlyph(ch);
        if (g != NULL)
            width += g->advance;
    }

    return width;
}


int TextureFont::getHeight() const
{
    return maxAscent + maxDescent;
}

int TextureFont::getMaxWidth() const
{
    return maxWidth;
}

int TextureFont::getMaxAscent() const
{
    return maxAscent;
}

void TextureFont::setMaxAscent(int _maxAscent)
{
    maxAscent = _maxAscent;
}

int TextureFont::getMaxDescent() const
{
    return maxDescent;
}

void TextureFont::setMaxDescent(int _maxDescent)
{
    maxDescent = _maxDescent;
}


int TextureFont::getTextureName() const
{
    return texName;
}


void TextureFont::bind()
{
    if (texName != 0)
        glBindTexture(GL_TEXTURE_2D, texName);
}


void TextureFont::addGlyph(const TextureFont::Glyph& g)
{
    glyphs.insert(glyphs.end(), g);
    if (g.width > maxWidth)
        maxWidth = g.width;
}


const TextureFont::Glyph* TextureFont::getGlyph(wchar_t ch) const
{
    if (ch >= (wchar_t)glyphLookupTableSize)
        return NULL;
    else
        return glyphLookup[ch];
}


bool TextureFont::buildTexture()
{
    assert(fontImage != NULL);

    if (texName != 0)
        glDeleteTextures(1, (const GLuint*) &texName);
    glGenTextures(1, (GLuint*) &texName);
    if (texName == 0)
    {
        DPRINTF(0, "Failed to allocate texture object for font.\n");
        return false;
    }

    glBindTexture(GL_TEXTURE_2D, texName);

    // Don't build mipmaps . . . should really make them an option.
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_ALPHA,
                 texWidth, texHeight,
                 0,
                 GL_ALPHA, GL_UNSIGNED_BYTE,
                 fontImage);

    return true;
}


void TextureFont::rebuildGlyphLookupTable()
{
    if (glyphs.size() == 0)
        return;

    // Find the largest glyph id
    int maxID = glyphs[0].__id;
    vector<Glyph>::const_iterator iter;
    for (iter = glyphs.begin(); iter != glyphs.end(); iter++)
    {
        if (iter->__id > maxID)
            maxID = iter->__id;
    }

    // If there was already a lookup table, delete it.
    if (glyphLookup != NULL)
        delete[] glyphLookup;

    DPRINTF(1, "texturefont: allocating glyph lookup table with %d entries.\n",
            maxID + 1);
    glyphLookup = new const Glyph*[maxID + 1];
    for (int i = 0; i <= maxID; i++)
        glyphLookup[i] = NULL;

    // Fill the table with glyph pointers
    for (iter = glyphs.begin(); iter != glyphs.end(); iter++)
        glyphLookup[iter->__id] = &(*iter);
    glyphLookupTableSize = (unsigned int) maxID + 1;
}


static uint32 readUint32(istream& in, bool swap)
{
    uint32 x;
    in.read(reinterpret_cast<char*>(&x), sizeof x);
    return swap ? bswap_32(x) : x;
}

static uint16 readUint16(istream& in, bool swap)
{
    uint16 x;
    in.read(reinterpret_cast<char*>(&x), sizeof x);
    return swap ? bswap_16(x) : x;
}

static uint8 readUint8(istream& in)
{
    uint8 x;
    in.read(reinterpret_cast<char*>(&x), sizeof x);
    return x;
}

/* Not currently used
static int32 readInt32(istream& in, bool swap)
{
    int32 x;
    in.read(reinterpret_cast<char*>(&x), sizeof x);
    return swap ? static_cast<int32>(bswap_32(static_cast<uint32>(x))) : x;
}*/

static int16 readInt16(istream& in, bool swap)
{
    int16 x;
    in.read(reinterpret_cast<char*>(&x), sizeof x);
    return swap ? static_cast<int16>(bswap_16(static_cast<uint16>(x))) : x;
}

static int8 readInt8(istream& in)
{
    int8 x;
    in.read(reinterpret_cast<char*>(&x), sizeof x);
    return x;
}


TextureFont* TextureFont::load(istream& in)
{
    char header[4];

    in.read(header, sizeof header);
    if (!in.good() || strncmp(header, "\377txf", 4) != 0)
    {
        DPRINTF(0, "Stream is not a texture font!.\n");
        return NULL;
    }

    uint32 endiannessTest = 0;
    in.read(reinterpret_cast<char*>(&endiannessTest), sizeof endiannessTest);
    if (!in.good())
    {
        DPRINTF(0, "Error reading endianness bytes in txf header.\n");
        return NULL;
    }

    bool byteSwap;
    if (endiannessTest == 0x78563412)
        byteSwap = true;
    else if (endiannessTest == 0x12345678)
        byteSwap = false;
    else
    {
        DPRINTF(0, "Stream is not a texture font!.\n");
        return NULL;
    }

    int format = readUint32(in, byteSwap);
    unsigned int texWidth = readUint32(in, byteSwap);
    unsigned int texHeight = readUint32(in, byteSwap);
    unsigned int maxAscent = readUint32(in, byteSwap);
    unsigned int maxDescent = readUint32(in, byteSwap);
    unsigned int nGlyphs = readUint32(in, byteSwap);

    if (!in)
    {
        DPRINTF(0, "Texture font stream is incomplete");
        return NULL;
    }

    DPRINTF(1, "Font contains %d glyphs.\n", nGlyphs);

    TextureFont* font = new TextureFont();
    assert(font != NULL);

    font->setMaxAscent(maxAscent);
    font->setMaxDescent(maxDescent);

    float dx = 0.5f / texWidth;
    float dy = 0.5f / texHeight;

    for (unsigned int i = 0; i < nGlyphs; i++)
    {
        uint16 __id = readUint16(in, byteSwap);
        TextureFont::Glyph glyph(__id);

        glyph.width = readUint8(in);
        glyph.height = readUint8(in);
        glyph.xoff = readInt8(in);
        glyph.yoff = readInt8(in);
        glyph.advance = readInt8(in);
        readInt8(in);
        glyph.x = readInt16(in, byteSwap);
        glyph.y = readInt16(in, byteSwap);
        
        if (!in)
        {
            DPRINTF(0, "Error reading glyph %ud from texture font stream.\n", i);
            delete font;
            return NULL;
        }

        float fWidth = texWidth;
        float fHeight = texHeight;
        glyph.texCoords[0].u = glyph.x / fWidth + dx;
        glyph.texCoords[0].v = glyph.y / fHeight + dy;
        glyph.texCoords[1].u = (glyph.x + glyph.width) / fWidth + dx;
        glyph.texCoords[1].v = glyph.y / fHeight + dy;
        glyph.texCoords[2].u = (glyph.x + glyph.width) / fWidth + dx;
        glyph.texCoords[2].v = (glyph.y + glyph.height) / fHeight + dy;
        glyph.texCoords[3].u = glyph.x / fWidth + dx;
        glyph.texCoords[3].v = (glyph.y + glyph.height) / fHeight + dy;

        font->addGlyph(glyph);
    }

    font->texWidth = texWidth;
    font->texHeight = texHeight;
    if (format == TxfByte)
    {
        unsigned char* fontImage = new unsigned char[texWidth * texHeight];
        if (fontImage == NULL)
        {
            DPRINTF(0, "Not enough memory for font bitmap.\n");
            delete font;
            return NULL;
        }

        DPRINTF(1, "Reading %d x %d 8-bit font image.\n", texWidth, texHeight);

        in.read(reinterpret_cast<char*>(fontImage), texWidth * texHeight);
        if (in.gcount() != (signed)(texWidth * texHeight))
        {
            DPRINTF(0, "Missing bitmap data in font stream.\n");
            delete font;
            delete[] fontImage;
            return NULL;
        }

        font->fontImage = fontImage;
    }
    else
    {
        int rowBytes = (texWidth + 7) >> 3;
        unsigned char* fontBits = new unsigned char[rowBytes * texHeight];
        unsigned char* fontImage = new unsigned char[texWidth * texHeight];
        if (fontImage == NULL || fontBits == NULL)
        {
            DPRINTF(0, "Not enough memory for font bitmap.\n");
            delete font;
            if (fontBits != NULL)
                delete[] fontBits;
            if (fontImage != NULL)
                delete[] fontImage;
            return NULL;
        }

        DPRINTF(1, "Reading %d x %d 1-bit font image.\n", texWidth, texHeight);

        in.read(reinterpret_cast<char*>(fontBits), rowBytes * texHeight);
        if (in.gcount() != (signed)(rowBytes * texHeight))
        {
            DPRINTF(0, "Missing bitmap data in font stream.\n");
            delete font;
            return NULL;
        }

        for (unsigned int y = 0; y < texHeight; y++)
        {
            for (unsigned int x = 0; x < texWidth; x++)
            {
                if ((fontBits[y * rowBytes + (x >> 3)] & (1 << (x & 0x7))) != 0)
                    fontImage[y * texWidth + x] = 0xff;
                else
                    fontImage[y * texWidth + x] = 0x0;
            }
        }

        font->fontImage = fontImage;
        delete[] fontBits;
    }

    font->rebuildGlyphLookupTable();

    return font;
}


TextureFont* LoadTextureFont(const string& filename)
{
    string localeFilename = LocaleFilename(filename);
    ifstream in(localeFilename.c_str(), ios::in | ios::binary);
    if (!in.good())
    {
        DPRINTF(0, "Could not open font file %s\n", filename.c_str());
        return NULL;
    }

    return TextureFont::load(in);
}
