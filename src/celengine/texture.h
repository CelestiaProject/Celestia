// texture.h
//
// Copyright (C) 2003-present, Celestia Development Team
// Copyright (C) 2001-2003, Chris Laurel
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#pragma once

#include <array>
#include <cstdint>
#include <filesystem>
#include <memory>
#include <string>

#include <Eigen/Core>

#include <celimage/image.h>
#include <celutil/array_view.h>
#include <celutil/classops.h>
#include <celutil/color.h>

typedef void (*ProceduralTexEval)(float, float, float, std::uint8_t*);

struct TextureTile
{
    TextureTile(unsigned int _texID) :
        texID(_texID) {}
    TextureTile(unsigned int _texID, float _u, float _v) :
        u(_u), v(_v), texID(_texID) {}
    TextureTile(unsigned int _texID, float _u, float _v, float _du, float _dv) :
        u(_u), v(_v), du(_du), dv(_dv), texID(_texID) {}

    float u{ 0.0f };
    float v{ 0.0f };
    float du{ 1.0f };
    float dv{ 1.0f };
    unsigned int texID;
};

class Texture : private celestia::util::NoCopy
{
public:
    virtual ~Texture() = default;

    virtual TextureTile getTile(int lod, int u, int v) = 0;
    virtual void bind() = 0;

    virtual int getLODCount() const;
    virtual int getUTileCount(int lod) const;
    virtual int getVTileCount(int lod) const;
    virtual int getWTileCount(int lod) const;

    // Currently, these methods are only implemented by virtual textures; they
    // may be useful later when a more sophisticated texture management scheme
    // is implemented.
    virtual void beginUsage() {};
    virtual void endUsage() {};

    virtual void setBorderColor(Color);

    int getWidth() const;
    int getHeight() const;

    bool hasAlpha() const { return alpha; }

    /*! Identical formats may need to be treated in slightly different
     *  fashions. One (and currently the only) example is the DXT5 compressed
     *  normal map format, which is an ordinary DXT5 texture but requires some
     *  shader tricks to be used correctly.
     */
    unsigned int getFormatOptions() const;

    //! Set the format options.
    void setFormatOptions(unsigned int opts);

    enum AddressMode
    {
        Wrap        = 0,
        BorderClamp = 1,
        EdgeClamp   = 2,
    };

    enum MipMapMode
    {
        DefaultMipMaps = 0,
        NoMipMaps      = 1,
    };

    // Format option flags
    enum
    {
        DXT5NormalMap = 1
    };

    enum Colorspace
    {
        DefaultColorspace = 0,
        LinearColorspace  = 1,
        sRGBColorspace    = 2
    };

protected:
    Texture(int _w, int _h, bool _alpha = false);

private:
    int width;
    int height;
    bool alpha;

    unsigned int formatOptions{ 0 };
};

class ImageTexture : public Texture
{
public:
    ImageTexture(const celestia::engine::Image& img, AddressMode, MipMapMode);
    ~ImageTexture();

    template<typename F>
    static std::unique_ptr<ImageTexture> createProcedural(int width, int height,
                                                          celestia::engine::PixelFormat format,
                                                          F func,
                                                          Texture::AddressMode addressMode = Texture::EdgeClamp,
                                                          Texture::MipMapMode mipMode = Texture::DefaultMipMaps);

    TextureTile getTile(int lod, int u, int v) override;
    void bind() override;
    void setBorderColor(Color) override;

    unsigned int getName() const;

private:
    unsigned int glName;
};

template<typename F>
std::unique_ptr<ImageTexture>
ImageTexture::createProcedural(int width, int height,
                               celestia::engine::PixelFormat format,
                               F func,
                               Texture::AddressMode addressMode,
                               Texture::MipMapMode mipMode)
{
    celestia::engine::Image img(format, width, height);
    const auto fwidth = static_cast<float>(width);
    const auto fheight = static_cast<float>(height);
    for (int y = 0; y < height; ++y)
    {
        float v = (static_cast<float>(y) + 0.5f) / fheight * 2 - 1;
        std::uint8_t* ptr = img.getPixelRow(y);
        for (int x = 0; x < width; ++x)
        {
            float u = (static_cast<float>(x) + 0.5f) / fwidth * 2 - 1;
            func(u, v, ptr);
            ptr += img.getComponents();
        }
    }

    return std::make_unique<ImageTexture>(img, addressMode, mipMode);
}

class TiledTexture : public Texture
{
public:
    TiledTexture(const celestia::engine::Image& img, int _uSplit, int _vSplit, MipMapMode);
    ~TiledTexture();

    TextureTile getTile(int lod, int u, int v) override;
    void bind() override;
    void setBorderColor(Color) override;

    int getUTileCount(int lod) const override;
    int getVTileCount(int lod) const override;

private:
    int uSplit;
    int vSplit;
    std::unique_ptr<unsigned int[]> glNames; //NOSONAR
};

class CubeMap : public Texture
{
public:
    explicit CubeMap(celestia::util::array_view<celestia::engine::Image>);
    ~CubeMap();

    template<typename F>
    static std::unique_ptr<CubeMap> createProcedural(int size,
                                                     celestia::engine::PixelFormat format,
                                                     F func);

    TextureTile getTile(int lod, int u, int v) override;
    void bind() override;
    void setBorderColor(Color) override;

private:
    static Eigen::Vector3f cubeVector(int face, float s, float t);

    unsigned int glName;
};

template<typename F>
std::unique_ptr<CubeMap>
CubeMap::createProcedural(int size, celestia::engine::PixelFormat format, F func)
{
    using celestia::engine::Image;
    std::array<Image, 6> faces
    {
        Image(format, size, size), Image(format, size, size), Image(format, size, size),
        Image(format, size, size), Image(format, size, size), Image(format, size, size),
    };

    const auto fsize = static_cast<float>(size);
    for (int i = 0; i < 6; ++i)
    {
        Image& face = faces[i];
        for (int y = 0; y < size; ++y)
        {
            float t = (static_cast<float>(y) + 0.5f) / fsize * 2 - 1;
            std::uint8_t* ptr = face.getPixelRow(y);
            for (int x = 0; x < size; ++x)
            {
                float s = (static_cast<float>(x) + 0.5f) / fsize * 2 - 1;
                Eigen::Vector3f v = cubeVector(i, s, t);
                func(v.x(), v.y(), v.z(), ptr);
                ptr += face.getComponents();
            }
        }
    }

    return std::make_unique<CubeMap>(faces);
}

std::unique_ptr<Texture>
LoadTextureFromFile(const std::filesystem::path& filename,
                    Texture::AddressMode addressMode = Texture::EdgeClamp,
                    Texture::MipMapMode mipMode = Texture::DefaultMipMaps,
                    Texture::Colorspace colorspace = Texture::DefaultColorspace);

std::unique_ptr<Texture>
LoadHeightMapFromFile(const std::filesystem::path& filename,
                      float height,
                      Texture::AddressMode addressMode = Texture::EdgeClamp);
