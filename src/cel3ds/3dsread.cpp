// 3dsread.cpp
//
// Copyright (C) 2000, Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <iostream>
#include <string>
#include <utility>

#include <Eigen/Core>
#include <fmt/ostream.h>

#include <celutil/binaryread.h>
#include "3dschunk.h"
#include "3dsmodel.h"
#include "3dsread.h"

namespace celutil = celestia::util;

namespace
{
constexpr std::int32_t READ_FAILURE = -1;
constexpr std::int32_t UNKNOWN_CHUNK = -2;


template<typename T>
using ProcessChunkFunc = std::int32_t (*)(std::istream &, std::uint16_t, std::int32_t, T*);


std::int32_t readString(std::istream& in, std::string& value)
{
    constexpr std::size_t maxLength = 1024;
    char s[maxLength];

    for (std::size_t count = 0; count < maxLength; count++)
    {
        in.read(s + count, 1);
        if (!in.good()) { return READ_FAILURE; }
        if (s[count] == '\0')
        {
            value = s;
            return count + 1;
        }
    }

    return READ_FAILURE;
}


template<typename T>
std::int32_t read3DSChunk(std::istream& in,
                          ProcessChunkFunc<T> chunkFunc,
                          T* obj)
{
    std::uint16_t chunkType;
    if (!celutil::readLE<std::uint16_t>(in, chunkType)) { return READ_FAILURE; }
    std::int32_t chunkSize;
    if (!celutil::readLE<std::int32_t>(in, chunkSize) || chunkSize < 6) { return READ_FAILURE; }

    std::int32_t contentSize = chunkSize - 6;
    std::int32_t processedSize = chunkFunc(in, chunkType, contentSize, obj);
    switch (processedSize)
    {
    case READ_FAILURE:
        return READ_FAILURE;
    case UNKNOWN_CHUNK:
        in.ignore(contentSize);
        return in.good() ? chunkSize : READ_FAILURE;
    default:
        if (processedSize != contentSize)
        {
            fmt::print(std::clog, "Chunk type {:04x}, expected {} bytes but read {}\n", chunkType, contentSize, processedSize);
            return READ_FAILURE;
        }
        return chunkSize;
    }
}


template<typename T>
std::int32_t read3DSChunks(std::istream& in,
                           std::int32_t nBytes,
                           ProcessChunkFunc<T> chunkFunc,
                           T* obj)
{
    std::int32_t bytesRead = 0;

    while (bytesRead < nBytes)
    {
        std::int32_t chunkSize = read3DSChunk(in, chunkFunc, obj);
        if (chunkSize < 0) {
            fmt::print(std::clog, "Failed to read 3DS chunk\n");
            return READ_FAILURE;
        }
        bytesRead += chunkSize;
    }

    if (bytesRead != nBytes)
    {
        fmt::print(std::clog, "Multiple chunks, expected {} bytes but read {}\n", nBytes, bytesRead);
        return READ_FAILURE;
    }

    return bytesRead;
}


std::int32_t readColor(std::istream& in, M3DColor& color)
{
    std::uint8_t r, g, b;
    if (!celutil::readLE<std::uint8_t>(in, r)
        || !celutil::readLE<std::uint8_t>(in, g)
        || !celutil::readLE<std::uint8_t>(in, b))
    {
        return READ_FAILURE;
    }

    color = {static_cast<float>(r) / 255.0f,
             static_cast<float>(g) / 255.0f,
             static_cast<float>(b) / 255.0f};

    return 3;
}


std::int32_t readFloatColor(std::istream& in, M3DColor& color)
{
    float r, g, b;
    if (!celutil::readLE<float>(in, r)
        || !celutil::readLE<float>(in, g)
        || !celutil::readLE<float>(in, b))
    {
        return READ_FAILURE;
    }

    color = { r, g, b };
    return static_cast<std::int32_t>(3 * sizeof(float));
}


std::int32_t readMeshMatrix(std::istream& in, Eigen::Matrix4f& m)
{
    float elements[12];
    for (std::size_t i = 0; i < 12; ++i)
    {
        if (!celutil::readLE<float>(in, elements[i])) { return READ_FAILURE; }
    }

    m << elements[0], elements[1], elements[2], 0,
         elements[3], elements[4], elements[5], 0,
         elements[6], elements[7], elements[8], 0,
         elements[9], elements[10], elements[11], 1;

    return static_cast<std::int32_t>(12 * sizeof(float));
}


std::int32_t readPointArray(std::istream& in, M3DTriangleMesh* triMesh)
{
    std::uint16_t nPoints;
    if (!celutil::readLE<std::uint16_t>(in, nPoints)) { return READ_FAILURE; }
    std::int32_t bytesRead = static_cast<std::int32_t>(sizeof(nPoints));

    for (int i = 0; i < static_cast<int>(nPoints); i++)
    {
        float x, y, z;
        if (!celutil::readLE<float>(in, x)
            || !celutil::readLE<float>(in, y)
            || !celutil::readLE<float>(in, z))
        {
            return READ_FAILURE;
        }
        bytesRead += static_cast<std::int32_t>(3 * sizeof(float));
        triMesh->addVertex(Eigen::Vector3f(x, y, z));
    }

    return bytesRead;
}


std::int32_t readTextureCoordArray(std::istream& in, M3DTriangleMesh* triMesh)
{
    std::int32_t bytesRead = 0;

    std::uint16_t nPoints;
    if (!celutil::readLE<std::uint16_t>(in, nPoints)) { return READ_FAILURE; }
    bytesRead += static_cast<std::int32_t>(sizeof(nPoints));

    for (int i = 0; i < static_cast<int>(nPoints); i++)
    {
        float u, v;
        if (!celutil::readLE<float>(in, u) || !celutil::readLE<float>(in, v))
        {
            return READ_FAILURE;
        }
        bytesRead += static_cast<std::int32_t>(2 * sizeof(float));
        triMesh->addTexCoord(Eigen::Vector2f(u, -v));
    }

    return bytesRead;
}


std::int32_t processFaceArrayChunk(std::istream& in,
                                   std::uint16_t chunkType,
                                   std::int32_t /*contentSize*/,
                                   M3DTriangleMesh* triMesh)
{
    std::int32_t bytesRead = 0;
    std::uint16_t nFaces;
    std::unique_ptr<M3DMeshMaterialGroup> matGroup;

    switch (chunkType)
    {
    case M3DCHUNK_MESH_MATERIAL_GROUP:
        matGroup = std::make_unique<M3DMeshMaterialGroup>();

        bytesRead = readString(in, matGroup->materialName);
        if (bytesRead == READ_FAILURE || !celutil::readLE<std::uint16_t>(in, nFaces))
        {
            return READ_FAILURE;
        }
        bytesRead += static_cast<std::int32_t>(sizeof(nFaces));

        for (std::uint16_t i = 0; i < nFaces; i++)
        {
            std::uint16_t faceIndex;
            if (!celutil::readLE<std::uint16_t>(in, faceIndex)) { return READ_FAILURE; }
            bytesRead += static_cast<std::int32_t>(sizeof(faceIndex));
            matGroup->faces.push_back(faceIndex);
        }

        triMesh->addMeshMaterialGroup(std::move(matGroup));

        return bytesRead;

    case M3DCHUNK_MESH_SMOOTH_GROUP:
        nFaces = triMesh->getFaceCount();

        for (std::uint16_t i = 0; i < nFaces; i++)
        {
            std::int32_t groups;
            if (!celutil::readLE<std::int32_t>(in, groups) || groups < 0){ return READ_FAILURE; }
            bytesRead += static_cast<std::int32_t>(sizeof(groups));
            triMesh->addSmoothingGroups(static_cast<std::uint32_t>(groups));
        }
        return bytesRead;

    default:
        return UNKNOWN_CHUNK;
    }
}


std::int32_t readFaceArray(std::istream& in, M3DTriangleMesh* triMesh, std::int32_t contentSize)
{
    std::uint16_t nFaces;
    if (!celutil::readLE<std::uint16_t>(in, nFaces)) { return READ_FAILURE; }
    std::int32_t bytesRead = static_cast<std::int32_t>(sizeof(nFaces));

    for (int i = 0; i < static_cast<int>(nFaces); i++)
    {
        std::uint16_t v0, v1, v2, flags;
        if (!celutil::readLE<std::uint16_t>(in, v0)
            || !celutil::readLE<std::uint16_t>(in, v1)
            || !celutil::readLE<std::uint16_t>(in, v2)
            || !celutil::readLE<std::uint16_t>(in, flags))
        {
            return READ_FAILURE;
        }
        bytesRead += static_cast<std::int32_t>(4 * sizeof(std::uint16_t));
        triMesh->addFace(v0, v1, v2);
    }

    if (bytesRead > contentSize) { return READ_FAILURE; }

    if (bytesRead < contentSize)
    {
        std::int32_t trailingSize = read3DSChunks(in,
                                                  contentSize - bytesRead,
                                                  processFaceArrayChunk,
                                                  triMesh);
        bytesRead += trailingSize;
    }

    return bytesRead;
}


std::int32_t processTriMeshChunk(std::istream& in,
                                 std::uint16_t chunkType,
                                 std::int32_t contentSize,
                                 M3DTriangleMesh* triMesh)
{
    switch (chunkType)
    {
    case M3DCHUNK_POINT_ARRAY:
        return readPointArray(in, triMesh);
    case M3DCHUNK_MESH_TEXTURE_COORDS:
        return readTextureCoordArray(in, triMesh);
    case M3DCHUNK_FACE_ARRAY:
        return readFaceArray(in, triMesh, contentSize);
    case M3DCHUNK_MESH_MATRIX:
        {
            Eigen::Matrix4f matrix;
            std::int32_t bytesRead = readMeshMatrix(in, matrix);
            if (bytesRead < 0) { return READ_FAILURE; }
            triMesh->setMatrix(matrix);
            return bytesRead;
        }
    default:
        return UNKNOWN_CHUNK;
    }
}


std::int32_t processModelChunk(std::istream& in,
                               std::uint16_t chunkType,
                               std::int32_t contentSize,
                               M3DModel* model)
{
    if (chunkType == M3DCHUNK_TRIANGLE_MESH)
    {
        auto triMesh = std::make_unique<M3DTriangleMesh>();
        std::int32_t bytesRead = read3DSChunks(in, contentSize, processTriMeshChunk, triMesh.get());
        if (bytesRead == READ_FAILURE) { return READ_FAILURE; }
        model->addTriMesh(std::move(triMesh));
        return bytesRead;
    }

    return UNKNOWN_CHUNK;
}


std::int32_t processColorChunk(std::istream& in,
                               std::uint16_t chunkType,
                               std::int32_t /*contentSize*/,
                               M3DColor* color)
{
    switch (chunkType)
    {
    case M3DCHUNK_COLOR_24:
        return readColor(in, *color);
    case M3DCHUNK_COLOR_FLOAT:
        return readFloatColor(in, *color);
    default:
        return UNKNOWN_CHUNK;
    }
}


std::int32_t processPercentageChunk(std::istream& in,
                                    std::uint16_t chunkType,
                                    std::int32_t /*contentSize*/,
                                    float* percent)
{
    switch (chunkType)
    {
    case M3DCHUNK_INT_PERCENTAGE:
        {
            std::int16_t value;
            if (!celutil::readLE<std::int16_t>(in, value)) { return READ_FAILURE; }
            *percent = static_cast<float>(value);
            return sizeof(value);
        }
    case M3DCHUNK_FLOAT_PERCENTAGE:
        return celutil::readLE<float>(in, *percent) ? sizeof(float) : READ_FAILURE;
    default:
        return UNKNOWN_CHUNK;
    }
}


std::int32_t processTexmapChunk(std::istream& in,
                                std::uint16_t chunkType,
                                std::int32_t /*contentSize*/,
                                M3DMaterial* material)
{
    if (chunkType == M3DCHUNK_MATERIAL_MAPNAME)
    {
        std::string name;
        std::int32_t bytesRead = readString(in, name);
        if (bytesRead < 0) { return READ_FAILURE; }
        material->setTextureMap(name);
        return bytesRead;
    }

    return UNKNOWN_CHUNK;
}


std::int32_t processMaterialChunk(std::istream& in,
                                  std::uint16_t chunkType,
                                  std::int32_t contentSize,
                                  M3DMaterial* material)
{
    std::int32_t bytesRead;
    std::string name;
    M3DColor color;
    float t;

    switch (chunkType)
    {
    case M3DCHUNK_MATERIAL_NAME:
        bytesRead = readString(in, name);
        if (bytesRead < 0) { return READ_FAILURE; }
        material->setName(name);
        return bytesRead;
    case M3DCHUNK_MATERIAL_AMBIENT:
        bytesRead = read3DSChunks(in, contentSize, processColorChunk, &color);
        if (bytesRead < 0) { return READ_FAILURE; }
        material->setAmbientColor(color);
        return bytesRead;
    case M3DCHUNK_MATERIAL_DIFFUSE:
        bytesRead = read3DSChunks(in, contentSize, processColorChunk, &color);
        if (bytesRead < 0) { return READ_FAILURE; }
        material->setDiffuseColor(color);
        return bytesRead;
    case M3DCHUNK_MATERIAL_SPECULAR:
        bytesRead = read3DSChunks(in, contentSize, processColorChunk, &color);
        if (bytesRead < 0) { return READ_FAILURE; }
        material->setSpecularColor(color);
        return bytesRead;
    case M3DCHUNK_MATERIAL_SHININESS:
        bytesRead = read3DSChunks(in, contentSize, processPercentageChunk, &t);
        if (bytesRead < 0) { return READ_FAILURE; }
        material->setShininess(t);
        return bytesRead;
    case M3DCHUNK_MATERIAL_TRANSPARENCY:
        bytesRead = read3DSChunks(in, contentSize, processPercentageChunk, &t);
        if (bytesRead < 0) { return READ_FAILURE; }
        material->setOpacity(1.0f - t / 100.0f);
        return bytesRead;
    case M3DCHUNK_MATERIAL_TEXMAP:
        return read3DSChunks(in, contentSize, processTexmapChunk, material);
    default:
        return UNKNOWN_CHUNK;
    }
}


std::int32_t processSceneChunk(std::istream& in,
                               std::uint16_t chunkType,
                               std::int32_t contentSize,
                               M3DScene* scene)
{
    std::int32_t bytesRead, chunksSize;
    std::unique_ptr<M3DModel> model;
    std::unique_ptr<M3DMaterial> material;
    M3DColor color;
    std::string name;

    switch (chunkType)
    {
    case M3DCHUNK_NAMED_OBJECT:
        bytesRead = readString(in, name);
        if (bytesRead < 0) { return READ_FAILURE; }
        model = std::make_unique<M3DModel>();
        model->setName(name);
        chunksSize = read3DSChunks(in,
                                   contentSize - bytesRead,
                                   processModelChunk,
                                   model.get());
        if (chunksSize < 0) { return READ_FAILURE; }
        scene->addModel(std::move(model));

        return bytesRead + chunksSize;
    case M3DCHUNK_MATERIAL_ENTRY:
        material = std::make_unique<M3DMaterial>();
        bytesRead = read3DSChunks(in,
                                  contentSize,
                                  processMaterialChunk,
                                  material.get());
        if (bytesRead < 0) { return READ_FAILURE; }
        scene->addMaterial(std::move(material));

        return bytesRead;
    case M3DCHUNK_BACKGROUND_COLOR:
        bytesRead = read3DSChunks(in, contentSize, processColorChunk, &color);
        if (bytesRead < 0) { return READ_FAILURE; }
        scene->setBackgroundColor(color);
        return bytesRead;
    default:
        return UNKNOWN_CHUNK;
    }
}


std::int32_t processTopLevelChunk(std::istream& in,
                                  std::uint16_t chunkType,
                                  std::int32_t contentSize,
                                  M3DScene* scene)
{
    if (chunkType == M3DCHUNK_MESHDATA)
    {
        return read3DSChunks(in, contentSize, processSceneChunk, scene);
    }

    return UNKNOWN_CHUNK;
}

} // end namespace


std::unique_ptr<M3DScene> Read3DSFile(std::istream& in)
{
    std::uint16_t chunkType;
    if (!celutil::readLE<std::uint16_t>(in, chunkType) || chunkType != M3DCHUNK_MAGIC)
    {
        fmt::print(std::clog, "Read3DSFile: Wrong magic number in header\n");
        return nullptr;
    }

    std::int32_t chunkSize;
    if (!celutil::readLE<std::int32_t>(in, chunkSize) || chunkSize < 6)
    {
        fmt::print(std::clog, "Read3DSFile: Error reading 3DS file top level chunk size\n");
        return nullptr;
    }

    fmt::print(std::clog, "3DS file, {} bytes\n", chunkSize + 6);

    auto scene = std::make_unique<M3DScene>();
    std::int32_t contentSize = chunkSize - 6;

    std::int32_t bytesRead = read3DSChunks(in, contentSize, processTopLevelChunk, scene.get());
    if (bytesRead < 0) { return nullptr; }
    if (bytesRead != contentSize)
    {
        return nullptr;
    }

    return scene;
}


std::unique_ptr<M3DScene> Read3DSFile(const fs::path& filename)
{
    std::ifstream in(filename.string(), std::ios::in | std::ios::binary);
    if (!in.good())
    {
        fmt::print(std::clog, "Read3DSFile: Error opening {}\n", filename);
        return nullptr;
    }

    std::unique_ptr<M3DScene> scene = Read3DSFile(in);
    in.close();
    return scene;
}


#if 0
int main(int argc, char* argv[])
{
    if (argc != 2)
    {
        cerr << "Usage: 3dsread <filename>\n";
        exit(1);
    }

    ifstream in(argv[1], ios::in | ios::binary);
    if (!in.good())
    {
        cerr << "Error opening " << argv[1] << '\n';
        exit(1);
    }
    else
    {
        read3DSFile(in);
        in.close();
    }

    return 0;
}
#endif
