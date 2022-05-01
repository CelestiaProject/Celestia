// 3dsread.cpp
//
// Copyright (C) 2000, Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <iostream>
#include <string>
#include <utility>

#include <Eigen/Core>
#include <fmt/format.h>

#include <celutil/binaryread.h>
#include <celutil/logger.h>
#include "3dsmodel.h"
#include "3dsread.h"

namespace celutil = celestia::util;
using celestia::util::GetLogger;

namespace
{
enum class M3DChunkType : std::uint16_t
{
    Null                 = 0x0000,
    Version              = 0x0002,
    ColorFloat           = 0x0010,
    Color24              = 0x0011,
    LinColorF            = 0x0013,
    IntPercentage        = 0x0030,
    FloatPercentage      = 0x0031,
    MasterScale          = 0x0100,

    BackgroundColor      = 0x1200,

    Meshdata             = 0x3d3d,
    MeshVersion          = 0x3d3e,

    NamedObject          = 0x4000,
    TriangleMesh         = 0x4100,
    PointArray           = 0x4110,
    PointFlagArray       = 0x4111,
    FaceArray            = 0x4120,
    MeshMaterialGroup    = 0x4130,
    MeshTextureCoords    = 0x4140,
    MeshSmoothGroup      = 0x4150,
    MeshMatrix           = 0x4160,
    Magic                = 0x4d4d,

    MaterialName         = 0xa000,
    MaterialAmbient      = 0xa010,
    MaterialDiffuse      = 0xa020,
    MaterialSpecular     = 0xa030,
    MaterialShininess    = 0xa040,
    MaterialShin2Pct     = 0xa041,
    MaterialTransparency = 0xa050,
    MaterialXpfall       = 0xa052,
    MaterialRefblur      = 0xa053,
    MaterialSelfIllum    = 0xa084,
    MaterialWiresize     = 0xa087,
    MaterialXpfallin     = 0xa08a,
    MaterialShading      = 0xa100,
    MaterialTexmap       = 0xa200,
    MaterialMapname      = 0xa300,
    MaterialEntry        = 0xafff,

    Kfdata               = 0xb000,
};


constexpr auto chunkHeaderSize = static_cast<std::int32_t>(sizeof(M3DChunkType) + sizeof(std::int32_t));

} // end unnamed namespace


template<>
struct fmt::formatter<M3DChunkType>
{
    constexpr auto parse(const format_parse_context& ctx) const -> decltype(ctx.begin()) {
        // we should validate the format here but exceptions are disabled
        return ctx.begin();
    }

    template<typename FormatContext>
    auto format(const M3DChunkType& chunkType, FormatContext& ctx) -> decltype(ctx.out()) {
        return format_to(ctx.out(), "{:04x}", static_cast<std::uint16_t>(chunkType));
    }
};


namespace
{

bool readChunkType(std::istream& in, M3DChunkType& chunkType)
{
    std::uint16_t value;
    if (!celutil::readLE<std::uint16_t>(in, value)) { return false; }
    chunkType = static_cast<M3DChunkType>(value);
    return true;
}


bool readString(std::istream& in, std::int32_t& contentSize, std::string& value)
{
    if (contentSize == 0)
    {
        value.clear();
        return true;
    }

    std::size_t max_length = std::min(64, contentSize);
    value.resize(max_length);
    in.getline(value.data(), max_length, '\0');
    if (!in.good())
    {
        GetLogger()->error("Error occurred reading string\n");
        return false;
    }

    value.resize(in.gcount() - 1);
    contentSize -= in.gcount();
    return true;
}


bool skipChunk(std::istream& in, M3DChunkType chunkType, std::int32_t contentSize)
{
    GetLogger()->debug("Skipping {} bytes of unknown/unexpected chunk type {}\n", contentSize, chunkType);
    in.ignore(contentSize);
    if (in.good() || in.eof()) { return true; }

    GetLogger()->error("Error skipping {} bytes of unknown/unexpected chunk type {}\n", contentSize, chunkType);
    return false;
}


bool skipTrailing(std::istream& in, std::int32_t contentSize)
{
    if (contentSize < 0)
    {
        GetLogger()->error("Negative trailing chunk size {} detected", contentSize);
        return false;
    }

    if (contentSize == 0) { return true; }

    GetLogger()->debug("Skipping {} trailing bytes\n", contentSize);
    in.ignore(contentSize);
    if (in.good() || in.eof()) { return true; }

    GetLogger()->error("Error skipping {} trailing bytes\n", contentSize);
    return false;
}


template<typename T, typename ProcessFunc>
bool readChunks(std::istream& in, std::int32_t contentSize, T& obj, ProcessFunc processChunk)
{
    while (contentSize > chunkHeaderSize)
    {
        if (in.eof())
        {
            GetLogger()->warn("Unexpected EOF detected, stopping processing\n");
            return true;
        }

        M3DChunkType chunkType;
        if (!readChunkType(in, chunkType))
        {
            GetLogger()->error("Failed to read chunk type\n");
            return false;
        }

        GetLogger()->debug("Found chunk type {}\n", chunkType);

        std::int32_t chunkSize;
        if (!celutil::readLE<std::int32_t>(in, chunkSize))
        {
            GetLogger()->error("Failed to read chunk size\n", chunkType);
            return false;
        }
        else if (chunkSize < chunkHeaderSize)
        {
            GetLogger()->error("Chunk size {} too small to include header\n", chunkSize);
            return false;
        }
        else if (chunkSize > contentSize)
        {
            GetLogger()->error("Chunk size {} exceeds remaining content size {} of outer chunk\n", chunkSize, contentSize);
            return false;
        }

        if (!processChunk(in, chunkType, chunkSize - chunkHeaderSize, obj))
        {
            GetLogger()->debug("Failed to process inner chunk\n");
            return false;
        }

        contentSize -= chunkSize;
    }

    return skipTrailing(in, contentSize);
}


bool readPointArray(std::istream& in, std::int32_t contentSize, M3DTriangleMesh& triMesh)
{
    constexpr auto headerSize = static_cast<std::int32_t>(sizeof(std::uint16_t));
    if (contentSize < headerSize)
    {
        GetLogger()->error("Content size {} too small to include point array count\n", contentSize);
        return false;
    }

    std::uint16_t nPoints;
    if (!celutil::readLE<std::uint16_t>(in, nPoints))
    {
        GetLogger()->error("Failed to read point array count\n");
        return false;
    }

    auto pointsCount = static_cast<std::int32_t>(nPoints);
    std::int32_t expectedSize = headerSize + pointsCount * static_cast<std::int32_t>(3 * sizeof(float));
    if (contentSize < expectedSize)
    {
        GetLogger()->error("Content size {} too small to include point array with {} entries", contentSize);
        return false;
    }

    for (std::int32_t i = 0; i < pointsCount; ++i)
    {
        Eigen::Vector3f vertex;
        if (!celutil::readLE<float>(in, vertex.x())
            || !celutil::readLE<float>(in, vertex.y())
            || !celutil::readLE<float>(in, vertex.z()))
        {
            GetLogger()->error("Failed to read entry {} of point array\n", i);
            return false;
        }

        triMesh.addVertex(vertex);
    }

    return skipTrailing(in, contentSize - expectedSize);
}


bool readTextureCoordArray(std::istream& in, std::int32_t contentSize, M3DTriangleMesh& triMesh)
{
    constexpr auto headerSize = static_cast<std::int32_t>(sizeof(std::uint16_t));
    if (contentSize < headerSize)
    {
        GetLogger()->error("Content size {} too small to include texture coord array count\n", contentSize);
        return false;
    }

    std::uint16_t nTexCoords;
    if (!celutil::readLE<std::uint16_t>(in, nTexCoords))
    {
        GetLogger()->error("Failed to read texture coord array count\n");
        return false;
    }

    auto texCoordsCount = static_cast<std::int32_t>(nTexCoords);
    std::int32_t expectedSize = headerSize + texCoordsCount * static_cast<std::int32_t>(2 * sizeof(float));
    if (contentSize < expectedSize)
    {
        GetLogger()->error("Content size {} too small to include texture coord array with {} entries\n", contentSize, nTexCoords);
        return false;
    }

    for (std::int32_t i = 0; i < texCoordsCount; ++i)
    {
        Eigen::Vector2f texCoord;
        if (!celutil::readLE<float>(in, texCoord.x())
            || !celutil::readLE<float>(in, texCoord.y()))
        {
            GetLogger()->error("Failed to read entry {} of texture coord array\n", i);
            return false;
        }

        texCoord.y() = -texCoord.y();
        triMesh.addTexCoord(texCoord);
    }

    return skipTrailing(in, contentSize - expectedSize);
}


bool readMeshMaterialGroup(std::istream& in, std::int32_t contentSize, M3DTriangleMesh& triMesh)
{
    M3DMeshMaterialGroup matGroup;
    if (!readString(in, contentSize, matGroup.materialName)) { return false; }
    constexpr auto headerSize = static_cast<std::int32_t>(sizeof(std::uint16_t));
    if (contentSize < headerSize)
    {
        GetLogger()->error("Remaining content size {} too small to include material group face array count\n", contentSize);
        return false;
    }

    std::uint16_t nFaces;
    if (!celutil::readLE<std::uint16_t>(in, nFaces))
    {
        GetLogger()->error("Failed to read material group face array count\n");
        return false;
    }

    auto faceCount = static_cast<std::int32_t>(nFaces);
    std::int32_t expectedSize = headerSize + faceCount * static_cast<std::int32_t>(sizeof(std::uint16_t));
    if (contentSize < expectedSize)
    {
        GetLogger()->error("Remaining content size {} too small to include material group face array with {} entries\n", contentSize, nFaces);
        return false;
    }

    for (std::int32_t i = 0; i < faceCount; ++i)
    {
        std::uint16_t faceIndex;
        if (!celutil::readLE(in, faceIndex))
        {
            GetLogger()->error("Failed to read entry {} of material group face array\n", i);
            return false;
        }

        matGroup.faces.push_back(faceIndex);
    }

    triMesh.addMeshMaterialGroup(std::move(matGroup));
    return skipTrailing(in, contentSize - expectedSize);
}


bool readMeshSmoothGroup(std::istream& in, std::int32_t contentSize, M3DTriangleMesh& triMesh)
{
    auto faceCount = static_cast<std::int32_t>(triMesh.getFaceCount());
    std::int32_t expectedSize = faceCount * static_cast<std::int32_t>(sizeof(std::int32_t));
    if (contentSize < expectedSize)
    {
        GetLogger()->error("Content size {} too small to include smoothing group array with {} entries\n", contentSize, faceCount);
        return false;
    }

    for (std::int32_t i = 0; i < faceCount; ++i)
    {
        std::int32_t groups;
        if (!celutil::readLE<std::int32_t>(in, groups))
        {
            GetLogger()->error("Failed to read entry {} of smoothing group array\n", i);
            return false;
        }

        if (groups < 0)
        {
            GetLogger()->error("Invalid smoothing group entry {}\n", groups);
            return false;
        }

        triMesh.addSmoothingGroups(static_cast<std::uint32_t>(groups));
    }

    return skipTrailing(in, contentSize - expectedSize);
}


bool processFaceArrayChunk(std::istream& in, M3DChunkType chunkType, std::int32_t contentSize, M3DTriangleMesh& triMesh)
{
    switch (chunkType)
    {
    case M3DChunkType::MeshMaterialGroup:
        GetLogger()->debug("Processing MeshMaterialGroup chunk\n");
        return readMeshMaterialGroup(in, contentSize, triMesh);

    case M3DChunkType::MeshSmoothGroup:
        GetLogger()->debug("Processing MeshSmoothGroup chunk\n");
        return readMeshSmoothGroup(in, contentSize, triMesh);

    default:
        return skipChunk(in, chunkType, contentSize);
    }
}


bool readFaceArray(std::istream& in, std::int32_t contentSize, M3DTriangleMesh& triMesh)
{
    constexpr auto headerSize = static_cast<std::int32_t>(sizeof(std::uint16_t));
    if (contentSize < headerSize)
    {
        GetLogger()->error("Content size {} too small to include face array count\n", contentSize);
        return false;
    }

    std::uint16_t nFaces;
    if (!celutil::readLE<std::uint16_t>(in, nFaces))
    {
        GetLogger()->error("Failed to read face array count\n");
        return false;
    }

    auto faceCount = static_cast<std::int32_t>(nFaces);
    std::int32_t expectedSize = headerSize + faceCount * static_cast<std::int32_t>(4 * sizeof(std::uint16_t));
    if (contentSize < expectedSize)
    {
        GetLogger()->error("Content size {} too small to include face array with {} entries\n", contentSize, nFaces);
        return false;
    }

    for (std::int32_t i = 0; i < faceCount; ++i)
    {
        std::uint16_t v0, v1, v2, flags;
        if (!celutil::readLE<std::uint16_t>(in, v0)
            || !celutil::readLE<std::uint16_t>(in, v1)
            || !celutil::readLE<std::uint16_t>(in, v2)
            || !celutil::readLE<std::uint16_t>(in, flags))
        {
            GetLogger()->error("Failed to read entry {} of face array\n", i);
            return false;
        }

        triMesh.addFace(v0, v1, v2);
    }

    if (expectedSize < contentSize)
    {
        return readChunks(in, contentSize - expectedSize, triMesh, processFaceArrayChunk);
    }

    return true;
}


bool readMeshMatrix(std::istream& in, std::int32_t contentSize, M3DTriangleMesh& triMesh)
{
    constexpr auto expectedSize = static_cast<std::int32_t>(sizeof(float) * 12);
    if (contentSize < expectedSize)
    {
        GetLogger()->error("Content size {} too small to include mesh matrix\n", contentSize);
        return false;
    }

    float elements[12];
    for (std::size_t i = 0; i < 12; ++i)
    {
        if (!celutil::readLE<float>(in, elements[i]))
        {
            GetLogger()->error("Failed to read element {} of mesh matrix\n", i);
            return false;
        }
    }

    Eigen::Matrix4f matrix;
    matrix << elements[0], elements[1], elements[2], 0,
                elements[3], elements[4], elements[5], 0,
                elements[6], elements[7], elements[8], 0,
                elements[9], elements[10], elements[11], 1;
    triMesh.setMatrix(matrix);

    return skipTrailing(in, contentSize - expectedSize);
}


bool processTriangleMeshChunk(std::istream& in, M3DChunkType chunkType, std::int32_t contentSize, M3DTriangleMesh& triMesh)
{
    switch (chunkType)
    {
    case M3DChunkType::PointArray:
        GetLogger()->debug("Processing PointArray chunk\n");
        return readPointArray(in, contentSize, triMesh);

    case M3DChunkType::MeshTextureCoords:
        GetLogger()->debug("Processing MeshTextureCoords chunk\n");
        return readTextureCoordArray(in, contentSize, triMesh);

    case M3DChunkType::FaceArray:
        GetLogger()->debug("Processing FaceArray chunk\n");
        return readFaceArray(in, contentSize, triMesh);

    case M3DChunkType::MeshMatrix:
        GetLogger()->debug("Processing MeshMatrix chunk\n");
        return readMeshMatrix(in, contentSize, triMesh);

    default:
        return skipChunk(in, chunkType, contentSize);
    }
}


bool processModelChunk(std::istream& in, M3DChunkType chunkType, std::int32_t contentSize, M3DModel& model)
{
    if (chunkType != M3DChunkType::TriangleMesh)
    {
        return skipChunk(in, chunkType, contentSize);
    }

    GetLogger()->debug("Processing TriangleMesh chunk\n");
    M3DTriangleMesh triMesh;
    if (!readChunks(in, contentSize, triMesh, processTriangleMeshChunk)) { return false; }
    model.addTriMesh(std::move(triMesh));
    return true;
}


bool readColor24(std::istream& in, std::int32_t contentSize, M3DColor& color)
{
    if (contentSize < 3)
    {
        GetLogger()->warn("Content size {} too small to include 24-bit color\n", contentSize);
        return skipTrailing(in, contentSize);
    }

    std::array<char, 3> rgb;
    if (!in.read(rgb.data(), rgb.size()).good())
    {
        GetLogger()->error("Error reading Color24 RGB values");
        return false;
    }

    color.red = static_cast<float>(static_cast<std::uint8_t>(rgb[0])) / 255.0f;
    color.green = static_cast<float>(static_cast<std::uint8_t>(rgb[1])) / 255.0f;
    color.blue = static_cast<float>(static_cast<std::uint8_t>(rgb[2])) / 255.0f;

    return skipTrailing(in, contentSize - 3);
}


bool readColorFloat(std::istream& in, std::int32_t contentSize, M3DColor& color)
{
    constexpr auto expectedSize = static_cast<std::int32_t>(sizeof(float) * 3);
    GetLogger()->debug("Processing ColorFloat chunk\n");
    if (contentSize < expectedSize)
    {
        GetLogger()->warn("Content size {} too small to include float color\n", contentSize);
        return skipTrailing(in, contentSize);
    }

    if (!celutil::readLE<float>(in, color.red)
        || !celutil::readLE<float>(in, color.green)
        || !celutil::readLE<float>(in, color.blue))
    {
        GetLogger()->error("Error reading ColorFloat RGB values");
        return false;
    }

    return skipTrailing(in, contentSize - expectedSize);
}


bool processColorChunk(std::istream& in, M3DChunkType chunkType, std::int32_t contentSize, M3DColor& color)
{
    switch (chunkType)
    {
    case M3DChunkType::Color24:
        GetLogger()->debug("Processing Color24 chunk\n");
        return readColor24(in, contentSize, color);

    case M3DChunkType::ColorFloat:
        GetLogger()->debug("Processing ColorFloat chunk\n");
        return readColorFloat(in, contentSize, color);

    default:
        GetLogger()->warn("Unknown color chunk type {}\n", chunkType);
        return skipChunk(in, chunkType, contentSize);
    }
}


bool readIntPercentage(std::istream& in, std::int32_t contentSize, float& percentage)
{
    constexpr auto expectedSize = static_cast<std::int32_t>(sizeof(std::int16_t));
    if (contentSize < expectedSize)
    {
        GetLogger()->warn("Content size {} too small to include integer perecentage\n", contentSize);
        return skipTrailing(in, contentSize);
    }

    std::int16_t value;
    if (!celutil::readLE<std::int16_t>(in, value))
    {
        GetLogger()->error("Error reading IntPercentage\n");
        return false;
    }

    percentage = static_cast<float>(value);
    return skipTrailing(in, contentSize - expectedSize);
}


bool readFloatPercentage(std::istream& in, std::int32_t contentSize, float& percentage)
{
    constexpr auto expectedSize = static_cast<std::int32_t>(sizeof(float));
    if (contentSize < expectedSize)
    {
        GetLogger()->warn("Content size {} too small to include float percentage\n", contentSize);
        return skipTrailing(in, contentSize);
    }

    if (!celutil::readLE<float>(in, percentage))
    {
        GetLogger()->error("Error reading FloatPercentage\n");
        return false;
    }

    return skipTrailing(in, contentSize - expectedSize);
}


bool processPercentageChunk(std::istream& in, M3DChunkType chunkType, std::int32_t contentSize, float& percentage)
{
    switch (chunkType)
    {
    case M3DChunkType::IntPercentage:
        GetLogger()->debug("Processing IntPercentage chunk\n");
        return readIntPercentage(in, contentSize, percentage);

    case M3DChunkType::FloatPercentage:
        GetLogger()->debug("Processing FloatPercentage chunk\n");
        readFloatPercentage(in, contentSize, percentage);

    default:
        GetLogger()->warn("Unknown percentage {}\n", chunkType);
        return skipChunk(in, chunkType, contentSize);
    }
}


bool processTexmapChunk(std::istream& in, M3DChunkType chunkType, std::int32_t contentSize, M3DMaterial& material)
{
    if (chunkType != M3DChunkType::MaterialMapname)
    {
        return skipChunk(in, chunkType, contentSize);
    }

    GetLogger()->debug("Processing MaterialMapname chunk\n");
    std::string name;
    if (!readString(in, contentSize, name)) { return false; }
    material.setTextureMap(name);
    return skipTrailing(in, contentSize);
}


bool readMaterialName(std::istream& in, std::int32_t contentSize, M3DMaterial& material)
{
    std::string name;
    if (!readString(in, contentSize, name)) { return false; }
    material.setName(std::move(name));
    return skipTrailing(in, contentSize);
}


template<typename Setter>
bool readMaterialColor(std::istream& in, std::int32_t contentSize, Setter setter)
{
    M3DColor color;
    if (!readChunks(in, contentSize, color, processColorChunk)) { return false; }
    setter(color);
    return true;
}


template<typename Setter>
bool readMaterialPercentage(std::istream& in, std::int32_t contentSize, Setter setter)
{
    float percentage = 0.0f;
    if (!readChunks(in, contentSize, percentage, processPercentageChunk)) { return false; }
    setter(percentage);
    return true;
}


bool processMaterialChunk(std::istream& in, M3DChunkType chunkType, std::int32_t contentSize, M3DMaterial& material)
{
    switch (chunkType)
    {
    case M3DChunkType::MaterialName:
        GetLogger()->debug("Processing MaterialName chunk\n");
        return readMaterialName(in, contentSize, material);

    case M3DChunkType::MaterialAmbient:
        GetLogger()->debug("Processing MaterialAmbient chunk\n");
        return readMaterialColor(in, contentSize, [&](M3DColor color) { material.setAmbientColor(color); });

    case M3DChunkType::MaterialDiffuse:
        GetLogger()->debug("Processing MaterialDiffuse chunk\n");
        return readMaterialColor(in, contentSize, [&](M3DColor color) { material.setDiffuseColor(color); });

    case M3DChunkType::MaterialSpecular:
        GetLogger()->debug("Processing MaterialSpecular chunk\n");
        return readMaterialColor(in, contentSize, [&](M3DColor color) { material.setSpecularColor(color); });

    case M3DChunkType::MaterialShininess:
        GetLogger()->debug("Processing MaterialShininess chunk\n");
        return readMaterialPercentage(in, contentSize, [&](float percentage) { material.setShininess(percentage); });

    case M3DChunkType::MaterialTransparency:
        GetLogger()->debug("Processing MaterialTransparency chunk\n");
        return readMaterialPercentage(in, contentSize,
                                      [&](float percentage) { material.setOpacity(1.0f - percentage / 100.0f); });

    case M3DChunkType::MaterialTexmap:
        GetLogger()->debug("Processing MaterialTexmap chunk\n");
        return readChunks(in, contentSize, material, processTexmapChunk);

    default:
        return skipChunk(in, chunkType, contentSize);
    }
}


bool readNamedObject(std::istream& in, std::int32_t contentSize, M3DScene& scene)
{
    std::string name;
    if (!readString(in, contentSize, name)) { return false; }
    M3DModel model;
    model.setName(name);
    if (!readChunks(in, contentSize, model, processModelChunk)) { return false; }
    scene.addModel(std::move(model));
    return true;
}


bool readMaterialEntry(std::istream& in, std::int32_t contentSize, M3DScene& scene)
{
    M3DMaterial material;
    if (!readChunks(in, contentSize, material, processMaterialChunk)) { return false; }
    scene.addMaterial(std::move(material));
    return true;
}


bool readBackgroundColor(std::istream& in, std::int32_t contentSize, M3DScene& scene)
{
    M3DColor color;
    if (!readChunks(in, contentSize, color, processColorChunk)) { return false; }
    scene.setBackgroundColor(color);
    return true;
}


bool processMeshdataChunk(std::istream& in, M3DChunkType chunkType, std::int32_t contentSize, M3DScene& scene)
{
    switch (chunkType)
    {
    case M3DChunkType::NamedObject:
        GetLogger()->debug("Processing NamedObject chunk\n");
        return readNamedObject(in, contentSize, scene);

    case M3DChunkType::MaterialEntry:
        GetLogger()->debug("Processing MaterialEntry chunk\n");
        return readMaterialEntry(in, contentSize, scene);

    case M3DChunkType::BackgroundColor:
        GetLogger()->debug("Processing BackgroundColor chunk\n");
        return readBackgroundColor(in, contentSize, scene);

    default:
        return skipChunk(in, chunkType, contentSize);
    }
}


bool processTopLevelChunk(std::istream& in, M3DChunkType chunkType, std::int32_t contentSize, M3DScene& scene)
{
    if (chunkType != M3DChunkType::Meshdata)
    {
        return skipChunk(in, chunkType, contentSize);
    }

    GetLogger()->debug("Processing Meshdata chunk\n");
    return readChunks(in, contentSize, scene, processMeshdataChunk);
}

} // end unnamed namespace


std::unique_ptr<M3DScene> Read3DSFile(std::istream& in)
{
    M3DChunkType chunkType;
    if (!readChunkType(in, chunkType) || chunkType != M3DChunkType::Magic)
    {
        GetLogger()->error("Read3DSFile: Wrong magic number in header\n");
        return nullptr;
    }

    std::int32_t chunkSize;
    if (!celutil::readLE<std::int32_t>(in, chunkSize) || chunkSize < chunkHeaderSize)
    {
        GetLogger()->error("Read3DSFile: Error reading 3DS file top level chunk size\n");
        return nullptr;
    }

    GetLogger()->verbose("3DS file, {} bytes\n", chunkSize + chunkHeaderSize);

    auto scene = std::make_unique<M3DScene>();
    if (!readChunks(in, chunkSize - chunkHeaderSize, *scene, processTopLevelChunk))
    {
        return nullptr;
    }

    return scene;
}


std::unique_ptr<M3DScene> Read3DSFile(const fs::path& filename)
{
    std::ifstream in(filename, std::ios::in | std::ios::binary);
    if (!in.good())
    {
        GetLogger()->error("Read3DSFile: Error opening {}\n", filename);
        return nullptr;
    }

    std::unique_ptr<M3DScene> scene = Read3DSFile(in);
    in.close();
    return scene;
}
