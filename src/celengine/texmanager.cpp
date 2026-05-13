// texmanager.cpp
//
// Copyright (C) 2001-present, Celestia Development Team
// Copyright (C) 2001 Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include "texmanager.h"

#include <array>
#include <cstddef>
#include <fstream>
#include <string_view>
#include <system_error>
#include <tuple>

#include <boost/container_hash/hash.hpp>

#include <celutil/logger.h>
#include "texture.h"

using namespace std::string_view_literals;
using celestia::util::GetLogger;

namespace celestia::engine
{
namespace
{
const std::filesystem::path baseDirectory = "textures";

const std::array<std::filesystem::path, 3> directories =
{
    baseDirectory / "lores",
    baseDirectory / "medres",
    baseDirectory / "hires",
};

constexpr std::array extensions =
{
#ifdef USE_LIBAVIF
    "avif"sv,
#endif
    "png"sv,
    "jpg"sv,
    "jpeg"sv,
    "dds"sv,
    "dxt5nm"sv,
    "ctx"sv,
};

std::unique_ptr<Texture>
LoadTexture(const TextureInfo& info)
{
    Texture::AddressMode addressMode = Texture::EdgeClamp;
    Texture::MipMapMode  mipMode     = Texture::DefaultMipMaps;
    Texture::Colorspace  colorspace  = Texture::DefaultColorspace;

    if (util::is_set(info.flags, TextureFlags::WrapTexture))
        addressMode = Texture::Wrap;
    else if (util::is_set(info.flags, TextureFlags::BorderClamp))
        addressMode = Texture::BorderClamp;

    if (util::is_set(info.flags, TextureFlags::NoMipMaps))
        mipMode = Texture::NoMipMaps;

    if (util::is_set(info.flags, TextureFlags::LinearColorspace))
        colorspace = Texture::LinearColorspace;

    if (info.bumpHeight == 0.0f)
    {
        GetLogger()->debug("Loading texture: {}\n", info.path);
        return LoadTextureFromFile(info.path, addressMode, mipMode, colorspace);
    }

    GetLogger()->debug("Loading bump map: {}\n", info.path);
    return LoadHeightMapFromFile(info.path, info.bumpHeight, addressMode);
}

} // end unnamed namespace

TexturePaths::Info::Info(PathSetIndex _pathSet,
                         TextureFlags _flags,
                         float _bumpHeight) :
    pathSet(_pathSet),
    flags(_flags),
    bumpHeight(_bumpHeight)
{
}

std::size_t
TexturePaths::InfoHash::operator()(const Info& info) const
{
    std::size_t seed = 0;
    boost::hash_combine(seed, info.pathSet);
    boost::hash_combine(seed, info.flags);
    boost::hash_combine(seed, info.bumpHeight);
    return seed;
}

bool
TexturePaths::InfoEqual::operator()(const Info& lhs, const Info& rhs) const
{
    return std::tie(lhs.pathSet, lhs.flags, lhs.bumpHeight) ==
           std::tie(rhs.pathSet, rhs.flags, rhs.bumpHeight);
}

util::TextureHandle
TexturePaths::getHandle(const std::filesystem::path& filename,
                        const std::filesystem::path& directory,
                        TextureFlags flags,
                        float bumpHeight)
{
    if (filename.empty())
        return util::TextureHandle::Invalid;

    PathSetIndex pathSetIndex = getPathSetIndex(filename, directory);
    if (pathSetIndex == PathSetIndex::Invalid)
        return util::TextureHandle::Invalid;

    Info info(pathSetIndex, flags, bumpHeight);
    auto [it, inserted] = m_handles.try_emplace(info, static_cast<util::TextureHandle>(m_info.size()));
    if (inserted)
        m_info.push_back(info);

    return it->second;
}

TexturePaths::PathSetIndex
TexturePaths::getPathSetIndex(const std::filesystem::path& filename,
                              const std::filesystem::path& directory)
{
    DirectoryPaths& dirPaths = m_dirPaths.try_emplace(directory).first->second;
    auto [it, inserted] = dirPaths.try_emplace(filename, PathSetIndex::Invalid);
    if (!inserted ||
        checkPath(filename, directory, it->second) ||
        directory.empty())
    {
        return it->second;
    }

    DirectoryPaths& rootPaths = m_dirPaths.try_emplace({}).first->second;
    auto [rootIt, rootInserted] = rootPaths.try_emplace(filename, PathSetIndex::Invalid);
    if (!rootInserted || checkPath(filename, {}, rootIt->second))
        it->second = rootIt->second;

    GetLogger()->error("Failed to resolve texture file set {}\n", filename);
    return it->second;
}

bool
TexturePaths::checkPath(const std::filesystem::path& filename,
                        const std::filesystem::path& directory,
                        PathSetIndex& pathSetIndex)
{
    bool isWildcard = filename.extension() == ".*";
    unsigned int found = 0U;
    PathSet pathSet;
    pathSet.fill(PathIndex::Invalid);

    for (unsigned int i = 0; i < ResolutionCount; ++i)
    {
        std::filesystem::path path = directory.empty()
            ? directories[i] / filename
            : directory / directories[i] / filename;

        if (isWildcard)
        {
            path = util::ResolveWildcard(path, extensions);
        }
        else
        {
            std::error_code ec;
            auto status = std::filesystem::status(path, ec);
            if (ec || !std::filesystem::is_regular_file(status))
                path.clear();
        }

        if (path.empty())
            continue;

        found |= 1U << i;
        pathSet[i] = static_cast<PathIndex>(m_paths.size());
        m_paths.push_back(path);
    }

    if (found == 0U)
        return false;

    // Fill in missing textures

    if ((found & 1U) == 0)
    {
        // lores not present: use medres if available, otherwise hires
        pathSet[0] = (found & 2U) ? pathSet[1] : pathSet[2];
    }

    if ((found & 2U) == 0)
    {
        // medres not present: use lores if available, otherwise hires
        pathSet[1] = (found & 1U) ? pathSet[0] : pathSet[2];
    }

    if ((found & 4U) == 0)
    {
        // hires not present: use medres if available, otherwise lores
        pathSet[2] = (found & 2U) ? pathSet[1] : pathSet[0];
    }

    pathSetIndex = static_cast<PathSetIndex>(m_pathSets.size());
    m_pathSets.push_back(pathSet);
    return true;
}

bool
TexturePaths::getInfo(util::TextureHandle handle,
                      TextureResolution resolution,
                      TextureInfo& info) const
{
    auto handleIdx = static_cast<std::size_t>(handle);
    if (handleIdx > m_info.size())
        return false;

    const auto& item = m_info[handleIdx];
    if (item.pathSet == PathSetIndex::Invalid)
        return false;

    const auto& pathSet = m_pathSets[static_cast<std::size_t>(item.pathSet)];
    PathIndex pathIndex = pathSet[static_cast<std::size_t>(resolution)];
    if (pathIndex == PathIndex::Invalid)
        return false;

    info.path = m_paths[static_cast<std::size_t>(pathIndex)];
    info.flags = item.flags;
    info.bumpHeight = item.bumpHeight;
    return true;
}

bool
TexturePaths::samePath(util::TextureHandle handle,
                       TextureResolution resolution1,
                       TextureResolution resolution2) const
{
    auto handleIdx = static_cast<std::size_t>(handle);
    if (handleIdx >= m_info.size())
        return true;

    const auto& item = m_info[handleIdx];
    if (item.pathSet == PathSetIndex::Invalid)
        return true;

    const auto& pathSet = m_pathSets[static_cast<std::size_t>(item.pathSet)];
    return pathSet[static_cast<std::size_t>(resolution1)] ==
           pathSet[static_cast<std::size_t>(resolution2)];
}

TextureManager::TextureManager(std::shared_ptr<const TexturePaths> paths,
                               TextureResolution resolution) :
    m_paths(paths),
    m_resolution(resolution)
{
}

Texture*
TextureManager::find(util::TextureHandle handle)
{
    if (handle == util::TextureHandle::Invalid)
        return nullptr;

    auto [it, inserted] = m_textures.try_emplace(handle);
    if (!inserted)
        return it->second.get();

    GetLogger()->info("Loading texture handle {}\n", static_cast<std::uint32_t>(handle));

    TextureInfo info;
    if (!m_paths->getInfo(handle, m_resolution, info))
        return nullptr;

    it->second = LoadTexture(info);
    return it->second.get();
}

Texture*
TextureManager::findShadow(util::TextureHandle handle)
{
    // shadow textures will only load the medres texture
    constexpr auto ShadowResolution = TextureResolution::medres;

    if (handle == util::TextureHandle::Invalid)
        return nullptr;

    auto [it, inserted] = m_shadowTextures.try_emplace(handle);
    if (!inserted)
        return it->second.get();

    if (m_resolution <= ShadowResolution ||
        m_paths->samePath(handle, m_resolution, ShadowResolution))
    {
        auto [mainIt, mainInserted] = m_textures.try_emplace(handle);
        if (!mainInserted)
        {
            it->second = mainIt->second;
            return it->second.get();
        }

        TextureInfo info;
        if (!m_paths->getInfo(handle, m_resolution, info))
            return nullptr;

        it->second = mainIt->second = LoadTexture(info);
        return it->second.get();
    }

    TextureInfo info;
    if (!m_paths->getInfo(handle, ShadowResolution, info))
        return nullptr;

    it->second = LoadTexture(info);
    return it->second.get();
}

void
TextureManager::resolution(TextureResolution resolution)
{
    if (resolution == m_resolution)
        return;

    m_textures.clear();
    m_shadowTextures.clear();
    m_resolution = resolution;
}

} // end namespace celestia::engine
