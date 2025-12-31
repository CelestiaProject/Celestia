// texmanager.h
//
// Copyright (C) 2001-present, Celestia Development Team
// Copyright (C) 2001 Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <memory>
#include <unordered_map>
#include <vector>

#include <celutil/classops.h>
#include <celutil/flag.h>
#include <celutil/fsutils.h>
#include <celutil/texhandle.h>
#include "texture.h"

namespace celestia::engine
{

enum class TextureResolution : int
{
    lores = 0,
    medres,
    hires,
};

enum class TextureFlags : unsigned int
{
    None             = 0,
    WrapTexture      = 0x1,
    NoMipMaps        = 0x2,
    BorderClamp      = 0x4,
    LinearColorspace = 0x8,
};

ENUM_CLASS_BITWISE_OPS(TextureFlags)

struct TextureInfo
{
    std::filesystem::path path;
    TextureFlags flags;
    float bumpHeight;
};

class TexturePaths : private util::NoCopy
{
public:
    util::TextureHandle getHandle(const std::filesystem::path& filename,
                                  const std::filesystem::path& directory,
                                  TextureFlags flags = TextureFlags::None,
                                  float bumpHeight = 0.0f);
    bool getInfo(util::TextureHandle, TextureResolution, TextureInfo&) const;
    bool samePath(util::TextureHandle, TextureResolution, TextureResolution) const;

private:
    static constexpr inline auto ResolutionCount = 3U;

    enum class PathIndex : std::uint32_t { Invalid = ~UINT32_C(0) };
    enum class PathSetIndex : std::uint32_t { Invalid = ~UINT32_C(0) };

    using PathSet = std::array<PathIndex, ResolutionCount>;

    struct Info
    {
        Info(PathSetIndex, TextureFlags, float);

        PathSetIndex pathSet;
        TextureFlags flags;
        float bumpHeight;
    };

    struct InfoHash
    {
        std::size_t operator()(const Info&) const;
    };

    struct InfoEqual
    {
        bool operator()(const Info&, const Info&) const;
    };

    using DirectoryPaths = std::unordered_map<std::filesystem::path, PathSetIndex, util::PathHasher>;

    PathSetIndex getPathSetIndex(const std::filesystem::path&, const std::filesystem::path&);
    bool checkPath(const std::filesystem::path&, const std::filesystem::path&, PathSetIndex&);

    std::vector<std::filesystem::path> m_paths;
    std::vector<PathSet> m_pathSets;
    std::vector<Info> m_info;

    std::unordered_map<std::filesystem::path, DirectoryPaths, util::PathHasher> m_dirPaths;
    std::unordered_map<Info, util::TextureHandle, InfoHash, InfoEqual> m_handles;
};

class TextureManager
{
public:
    TextureManager(std::shared_ptr<const TexturePaths>, TextureResolution);

    Texture* find(util::TextureHandle handle);
    Texture* findShadow(util::TextureHandle handle);
    TextureResolution resolution() const noexcept { return m_resolution; }
    void resolution(TextureResolution);

private:
    std::shared_ptr<const TexturePaths> m_paths;
    std::unordered_map<util::TextureHandle, std::shared_ptr<Texture>> m_textures;
    std::unordered_map<util::TextureHandle, std::shared_ptr<Texture>> m_shadowTextures;
    TextureResolution m_resolution;
};

} // end namespace celestia::engine
