// truetypefont.cpp
//
// Copyright (C) 2019-2022, Celestia Development Team
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include "truetypefont.h"

#include <algorithm>
#include <array>
#include <cstddef>
#include <map>
#include <memory>
#include <system_error>
#include <vector>

#include <celcompat/charconv.h>
#include <celengine/glsupport.h>
#include <celengine/render.h>
#include <celengine/texture.h>
#include <celimage/image.h>
#include <celrender/gl/buffer.h>
#include <celrender/gl/vertexobject.h>
#include <celutil/logger.h>
#include <celutil/utf8.h>
#include <ft2build.h>
#include FT_FREETYPE_H

#define DUMP_TEXTURE 0

#if DUMP_TEXTURE
#include <fstream>
#endif

using celestia::compat::from_chars;
using celestia::util::GetLogger;
namespace gl = celestia::gl;

namespace
{

struct Glyph
{
    FT_ULong ch;

    int ax; // advance.x
    int ay; // advance.y

    unsigned int bw; // bitmap.width;
    unsigned int bh; // bitmap.height;

    int bl; // bitmap_left;
    int bt; // bitmap_top;

    float tx; // x offset of glyph in texture coordinates
    float ty; // y offset of glyph in texture coordinates
};

struct UnicodeBlock
{
    FT_ULong first;
    FT_ULong last;
};

constexpr Glyph g_badGlyph = { 0, 0, 0, 0, 0, 0, 0, 0.0f, 0.0f };
constexpr auto INVALID_POS = static_cast<std::size_t>(-1);

} // end unnamed namespace

struct TextureFontPrivate
{
    struct FontVertex
    {
        FontVertex(float _x, float _y, float _u, float _v) : x(_x), y(_y), u(_u), v(_v)
        {
        }
        float x, y;
        float u, v;
    };

    static_assert(std::is_standard_layout_v<FontVertex>);

    TextureFontPrivate(const Renderer *renderer);
    ~TextureFontPrivate();
    TextureFontPrivate() = delete;
    TextureFontPrivate(const TextureFontPrivate &) = delete;
    TextureFontPrivate(TextureFontPrivate &&) = default;
    TextureFontPrivate &operator=(const TextureFontPrivate &) = delete;
    TextureFontPrivate &operator=(TextureFontPrivate &&) = default;

    std::pair<float, float> render(std::u16string_view line, float x, float y);

    bool                       buildAtlas();
    void                       computeTextureSize();
    bool                       loadGlyphInfo(FT_ULong /*ch*/, Glyph & /*c*/) const;
    void                       initCommonGlyphs();
    int                        getCommonGlyphsCount();
    const Glyph &              getGlyph(std::int32_t /*ch*/, char16_t /*fallback*/);
    const Glyph &              getGlyph(FT_ULong /* ch */);
    [[nodiscard]] std::size_t  toPos(FT_ULong /*ch*/) const;
    void                       optimize();
    CelestiaGLProgram         *getProgram();
    void                       flush();

    const Renderer    *m_renderer;
    CelestiaGLProgram *m_prog{ nullptr };

    FT_Face m_face; // font face

    int m_maxAscent{ 0 };
    int m_maxDescent{ 0 };
    int m_maxWidth{ 0 };

    int m_texWidth{ 0 };
    int m_texHeight{ 0 };

    std::unique_ptr<ImageTexture> m_tex; // texture object

    std::vector<Glyph> m_glyphs; // character information

    std::array<UnicodeBlock, 2> m_unicodeBlocks;

    int m_commonGlyphsCount{ 0 };
    int m_inserted{ 0 };

    Eigen::Matrix4f m_projection;
    Eigen::Matrix4f m_modelView;

    std::vector<FontVertex> m_fontVertices;

    gl::VertexObject m_vao;
    gl::Buffer       m_vbo{ gl::Buffer::TargetHint::Array };
    gl::Buffer       m_vio{ gl::Buffer::TargetHint::ElementArray };

    bool m_shaderInUse{ false };

    static constexpr std::size_t MaxVertices = 256; // This gives BO size 4kB, MUST be multiply of 4
    static constexpr std::size_t MaxIndices = MaxVertices / 4 * 6;
};


TextureFontPrivate::TextureFontPrivate(const Renderer *renderer) : m_renderer(renderer)
{
    m_unicodeBlocks[0] = { 0x0020, 0x007E }; // Basic Latin
    m_unicodeBlocks[1] = { 0x03B1, 0x03CF }; // Lower case Greek

    m_vao.addVertexBuffer(
        m_vbo,
        CelestiaGLProgram::VertexCoordAttributeIndex,
        2,
        gl::VertexObject::DataType::Float,
        false,
        sizeof(FontVertex),
        offsetof(FontVertex, x));
    m_vao.addVertexBuffer(
        m_vbo,
        CelestiaGLProgram::TextureCoord0AttributeIndex,
        2,
        gl::VertexObject::DataType::Float,
        false,
        sizeof(FontVertex),
        offsetof(FontVertex, u));
    m_vao.setIndexBuffer(m_vio, 0, gl::VertexObject::IndexType::UnsignedShort);
}

TextureFontPrivate::~TextureFontPrivate()
{
    if (m_face != nullptr)
        FT_Done_Face(m_face);
}

bool
TextureFontPrivate::loadGlyphInfo(FT_ULong ch, Glyph &c) const
{
    FT_GlyphSlot g = m_face->glyph;
    if (FT_Load_Char(m_face, ch, FT_LOAD_RENDER) != 0)
    {
        c.ch = 0;
        return false;
    }

    c.ch = ch;
    c.ax = g->advance.x >> 6;
    c.ay = g->advance.y >> 6;
    c.bw = g->bitmap.width;
    c.bh = g->bitmap.rows;
    c.bl = g->bitmap_left;
    c.bt = g->bitmap_top;
    return true;
}

void
TextureFontPrivate::initCommonGlyphs()
{
    if (!m_glyphs.empty())
        return;

    m_glyphs.reserve(256);

    for (auto const &block : m_unicodeBlocks)
    {
        for (FT_ULong ch = block.first, e = block.last; ch <= e; ch++)
        {
            Glyph c;
            if (!loadGlyphInfo(ch, c))
                GetLogger()->warn("Loading character {:x} failed!\n", static_cast<unsigned>(ch));
            m_glyphs.push_back(c); // still pushing empty
        }
    }
}

void
TextureFontPrivate::computeTextureSize()
{
    int roww = 0;
    int rowh = 0;
    int w    = 0;
    int h    = 0;

    // Find minimum size for a texture holding all visible ASCII characters
    for (const auto &c : m_glyphs)
    {
        if (c.ch == 0) continue; // skip bad glyphs

        if (roww + static_cast<int>(c.bw) + 1 >= celestia::gl::maxTextureSize)
        {
            w = std::max(w, roww);
            h += rowh;
            roww = 0;
            rowh = 0;
        }
        roww += c.bw + 1;
        rowh = std::max(rowh, static_cast<int>(c.bh));
    }

    w = std::max(w, roww);
    h += rowh;

    m_texWidth  = w;
    m_texHeight = h;
}

bool
TextureFontPrivate::buildAtlas()
{
    initCommonGlyphs();
    computeTextureSize();

    // Create an image that will be used to hold all glyphs
    auto img = std::make_unique<Image>(celestia::PixelFormat::LUMINANCE, m_texWidth, m_texHeight);

    // Paste all glyph bitmaps into the texture, remembering the offset
    int ox = 0;
    int oy = 0;
    int rowh = 0;

    FT_GlyphSlot g = m_face->glyph;
    for (auto &c : m_glyphs)
    {
        if (c.ch == 0)
            continue; // skip bad glyphs

        if (FT_Load_Char(m_face, c.ch, FT_LOAD_RENDER) != 0)
        {
            GetLogger()->warn("Loading character {:x} failed!\n", static_cast<unsigned>(c.ch));
            c.ch = 0;
            continue;
        }

        // compute subimage position
        if (ox + int(g->bitmap.width) > int(m_texWidth))
        {
            oy += rowh;
            rowh = 0;
            ox   = 0;
        }

        // copy glyph image to the destination image
        auto bitmapRows = static_cast<int>(g->bitmap.rows);
        for (int y = 0; y < bitmapRows; y++)
        {
            std::uint8_t *dst = img->getPixelRow(oy + y) + ox * img->getComponents();
            const std::uint8_t *src = g->bitmap.buffer + y * g->bitmap.width;
            memcpy(dst, src, g->bitmap.width);
        }

        c.tx = static_cast<float>(ox) / static_cast<float>(m_texWidth);
        c.ty = static_cast<float>(oy) / static_cast<float>(m_texHeight);

        rowh = std::max(rowh, static_cast<int>(g->bitmap.rows));
        ox += g->bitmap.width + 1;
    }

    m_tex = std::make_unique<ImageTexture>(*img, Texture::EdgeClamp, Texture::NoMipMaps);

    return true;
}

int
TextureFontPrivate::getCommonGlyphsCount()
{
    if (m_commonGlyphsCount == 0)
    {
        for (auto const &block : m_unicodeBlocks)
            m_commonGlyphsCount += (block.last - block.first + 1);
    }
    return m_commonGlyphsCount;
}

std::size_t
TextureFontPrivate::toPos(FT_ULong ch) const
{
    std::size_t pos = 0;

    if (ch > m_unicodeBlocks.back().last)
        return INVALID_POS;

    for (const auto &r : m_unicodeBlocks)
    {
        if (ch < r.first)
            return INVALID_POS;

        if (ch <= r.last)
            return pos + ch - r.first;

        pos += r.last - r.first + 1;
    }
    return INVALID_POS;
}

const Glyph &
TextureFontPrivate::getGlyph(std::int32_t ch, char16_t fallback)
{
    if (ch >= 0 && ch < 0x110000)
    {
        auto ulch = static_cast<FT_ULong>(ch);
        const Glyph& g = getGlyph(ulch);
        if (g.ch == ulch)
            return g;
    }

    return getGlyph(static_cast<FT_ULong>(fallback));
}

const Glyph &
TextureFontPrivate::getGlyph(FT_ULong ch)
{
    if (auto pos = toPos(ch); pos != INVALID_POS)
        return m_glyphs[pos];

    auto it = std::find_if(m_glyphs.cbegin() + getCommonGlyphsCount(),
                           m_glyphs.cend(),
                           [ch](const Glyph &g) { return g.ch == ch; });

    if (it != m_glyphs.end())
        return *it;

    Glyph c;
    if (!loadGlyphInfo(ch, c))
        return g_badGlyph;

    flush(); // render text to avoid garbled output due to changed texture

    m_glyphs.push_back(c);
    if (++m_inserted == 10) optimize();
    buildAtlas();

    return m_glyphs.back();
}

void
TextureFontPrivate::optimize()
{
    m_inserted = 0;
}

/*
 * Render text using the currently loaded font and currently set font size.
 * Rendering starts at coordinates (x, y), z is always 0.
 * The pixel coordinates that the FreeType2 library uses are scaled by (sx, sy).
 */
std::pair<float, float>
TextureFontPrivate::render(std::u16string_view line, float x, float y)
{
    if (m_tex == nullptr)
        return {0.0f, 0.0f};

    // Use the texture containing the atlas
    m_tex->bind();

    std::u16string_view::size_type i = 0;
    while (i < line.size())
    {
        std::int32_t ch;
        if (line[i] < 0xd800 || line[i] >= 0xe000)
        {
            // BMP character: one UTF-16 unit
            ch = static_cast<std::int32_t>(line[i]);
            ++i;
        }
        else if (line[i] < 0xdc00 && (i + 1) < line.size() &&
            line[i + 1] >= 0xdc00 && line[i + 1] < 0xe000)
        {
            // Decode surrogate pair
            ch = (((static_cast<std::int32_t>(line[i]) - 0xd7c0) << 10) |
                  (static_cast<std::int32_t>(line[i + 1])));
            i += 2;
        }
        else
        {
            // Invalid surrogate pair
            ++i;
            continue;
        }
        auto &g = getGlyph(ch, u'?');

        // Calculate the vertex and texture coordinates
        const float x1 = x + g.bl;
        const float y1 = y + g.bt - g.bh;
        const float w  = g.bw;
        const float h  = g.bh;
        const float x2 = x1 + w;
        const float y2 = y1 + h;

        // Advance the cursor to the start of the next character
        x += g.ax;
        y += g.ay;

        // Skip glyphs that have no pixels
        if (g.bw == 0 || g.bh == 0) continue;

        const float tx1 = g.tx;
        const float ty1 = g.ty;
        const float tx2 = tx1 + w / m_texWidth;
        const float ty2 = ty1 + h / m_texHeight;

        m_fontVertices.emplace_back(x1, y1, tx1, ty2);
        m_fontVertices.emplace_back(x2, y1, tx2, ty2);
        m_fontVertices.emplace_back(x1, y2, tx1, ty1);
        m_fontVertices.emplace_back(x2, y2, tx2, ty1);

        if (m_fontVertices.size() == MaxVertices) flush();
    }

    return {x, y};
}

CelestiaGLProgram *
TextureFontPrivate::getProgram()
{
    if (m_prog == nullptr)
        m_prog = m_renderer->getShaderManager().getShader("text");
    return m_prog;
}

void
TextureFontPrivate::flush()
{
    if (m_fontVertices.size() < 4)
        return;

    std::vector<std::uint16_t> indexes;
    indexes.reserve(MaxIndices);
    for (std::uint16_t index = 0; index < static_cast<std::uint16_t>(m_fontVertices.size()); index += 4)
    {
        indexes.push_back(index + 0);
        indexes.push_back(index + 1);
        indexes.push_back(index + 2);
        indexes.push_back(index + 1);
        indexes.push_back(index + 3);
        indexes.push_back(index + 2);
    }

    m_vbo.bind().invalidateData().setData(m_fontVertices, gl::Buffer::BufferUsage::StreamDraw);
    m_vio.bind().invalidateData().setData(indexes, gl::Buffer::BufferUsage::StreamDraw);
    m_vao.draw(gl::VertexObject::Primitive::Triangles, static_cast<int>(indexes.size()));
    m_vbo.unbind();
    m_vio.unbind();

    m_fontVertices.clear();
}

TextureFont::TextureFont(const Renderer *renderer) :
    impl(std::make_unique<TextureFontPrivate>(renderer))
{
}

// Needs to have the definition of TextureFontPrivate visible when we define this
TextureFont::~TextureFont() = default;

/**
 * Render a string with the specified offset
 *
 * Render a string with the specified offset. Do *not* automatically update
 * the modelview transform.
 *
 * @param line -- line to render
 * @param xoffset -- horizontal offset
 * @param yoffset -- vertical offset
 * @return the start position for the next glyph
 */
std::pair<float, float>
TextureFont::render(std::u16string_view line, float xoffset, float yoffset) const
{
    return impl->render(line, xoffset, yoffset);
}

/**
 * Calculate string width in pixels
 *
 * Calculate string width using the current font.
 *
 * @param line -- string to calculate width
 * @return string width in pixels
 */
int
TextureFont::getWidth(std::u16string_view line) const
{
    int  width     = 0;
    for (auto ch : line)
    {
        auto &g = impl->getGlyph(ch, u'?');
        width += g.ax;
    }

    return width;
}

/**
 * Return line height for the current font as sum of the maximal ascent and the
 * maximal descent.
 */
int
TextureFont::getHeight() const
{
    return impl->m_maxAscent + impl->m_maxDescent;
}

/**
 * Return the maximal character width for the current font.
 */
int
TextureFont::getMaxWidth() const
{
    return impl->m_maxWidth;
}

/**
 * Return the maximal ascent for the current font.
 */
int
TextureFont::getMaxAscent() const
{
    return impl->m_maxAscent;
}

/**
 * Set the maximal ascent for the current font.
 */
void
TextureFont::setMaxAscent(int _maxAscent)
{
    impl->m_maxAscent = _maxAscent;
}

/**
 * Return the maximal descent for the current font.
 */
int
TextureFont::getMaxDescent() const
{
    return impl->m_maxDescent;
}

/**
 * Set the maximal descent for the current font.
 */
void
TextureFont::setMaxDescent(int _maxDescent)
{
    impl->m_maxDescent = _maxDescent;
}

/**
 * Use the current font for text rendering.
 */
void
TextureFont::bind()
{
    auto *prog = impl->getProgram();
    if (prog == nullptr || impl->m_tex == nullptr)
        return;

    glActiveTexture(GL_TEXTURE0);
    impl->m_tex->bind();
    prog->use();
    prog->samplerParam("atlasTex") = 0;
    prog->setMVPMatrices(impl->m_projection, impl->m_modelView);
    impl->m_shaderInUse = true;
}

/**
 * Assign Projection and ModelView matrices for the current font.
 */
void
TextureFont::setMVPMatrices(const Eigen::Matrix4f &p, const Eigen::Matrix4f &m)
{
    impl->m_projection = p;
    impl->m_modelView  = m;
    auto *prog         = impl->getProgram();
    if (prog != nullptr && impl->m_shaderInUse)
    {
        flush();
        prog->setMVPMatrices(p, m);
    }
}

/**
 * Stop the current font usage.
 */
void
TextureFont::unbind()
{
    flush();
    impl->m_shaderInUse = false;
}

/**
 * Perform all delayed text rendering operations.
 */
void
TextureFont::flush()
{
    impl->flush();
}

namespace
{

FT_Face
LoadFontFace(FT_Library ft, const fs::path &path, int index, int size, int dpi)
{
    FT_Face face;

    if (FT_New_Face(ft, path.string().c_str(), index, &face) != 0)
    {
        GetLogger()->error("Could not open font {}\n", path);
        return nullptr;
    }

    if (!FT_IS_SCALABLE(face))
    {
        GetLogger()->error("Font is not scalable: {}\n", path);
        FT_Done_Face(face);
        return nullptr;
    }

    if (FT_Set_Char_Size(face, 0, size << 6, dpi, dpi) != 0)
    {
        GetLogger()->error("Could not set font size {}\n", size);
        FT_Done_Face(face);
        return nullptr;
    }

    return face;
}

} // namespace

// temporary while no fontconfig support
fs::path
ParseFontName(const fs::path &filename, int &index, int &size)
{
    // Format with font path/collection index(if any)/font size(if any)
    auto fn = filename.string();
    if (auto ps = fn.rfind(','); ps != std::string::npos)
    {
        if (from_chars(&fn[ps + 1], &fn[fn.size()], size).ec == std::errc())
        {
            if (auto pi = fn.rfind(',', ps - 1); pi != std::string::npos)
            {
                if (from_chars(&fn[pi + 1], &fn[pi], index).ec == std::errc())
                    return fn.substr(0, pi);
            }
            return fn.substr(0, ps);
        }
    }
    return filename;
}

using FontCache = std::map<fs::path, std::weak_ptr<TextureFont>>;

std::shared_ptr<TextureFont>
LoadTextureFont(const Renderer *r, const fs::path &filename, int index, int size)
{
    // Init FreeType library
    static FT_Library ftlib = nullptr;
    if (ftlib == nullptr && FT_Init_FreeType(&ftlib) != 0)
    {
        GetLogger()->error("Could not init freetype library\n");
        return nullptr;
    }

    // Init FontCache
    static FontCache *fontCache = nullptr;
    if (fontCache == nullptr)
        fontCache = new FontCache;

    // Lookup for an existing cached font
    std::weak_ptr<TextureFont> &font = (*fontCache)[filename];
    std::shared_ptr<TextureFont> ret = font.lock();
    if (ret == nullptr)
    {
        int  psize    = TextureFont::kDefaultSize;
        int  pindex   = 0;
        auto nameonly = ParseFontName(filename, pindex, psize);
        auto face     = LoadFontFace(ftlib, nameonly,
                                     index > 0 ? index : pindex,
                                     size > 0 ? size : psize,
                                     r->getScreenDpi());
        if (face == nullptr)
            return nullptr;

        ret = std::make_shared<TextureFont>(r);
        ret->impl->m_face = face;

        if (!ret->impl->buildAtlas())
        {
            FT_Done_Face(face);
            return nullptr;
        }

        ret->setMaxAscent(static_cast<int>(face->size->metrics.ascender >> 6));
        ret->setMaxDescent(static_cast<int>(-face->size->metrics.descender >> 6));

        font = ret;
    }
    return ret;
}
