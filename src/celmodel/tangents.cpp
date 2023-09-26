// tangents.cpp
//
// Copyright (C) 2023-present, Celestia Development Team
// Original version (C) 2004-2010, Chris Laurel <claurel@gmail.com>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include <array>
#include <cassert>
#include <cstdint>
#include <cstring> // std::memcpy
#include <numeric> // std::iota, std::accumulate
#include <vector>
#include <utility>
#include <Eigen/Core>
#include <celmodel/mesh.h>
#include <celutil/array_view.h>
#include <celutil/logger.h>

using celestia::util::GetLogger;
using celestia::util::array_view;

namespace
{

struct Face
{
    Eigen::Vector3f normal;
    std::array<cmod::Index32, 3> i;    // vertex attribute indices
};

void
copyVertex(cmod::VWord* newVertexData,
           const cmod::VertexDescription& newDesc,
           const cmod::VWord* oldVertexData,
           const cmod::VertexDescription& oldDesc,
           cmod::Index32 oldIndex,
           array_view<std::uint32_t> fromOffsets)
{
    unsigned int stride = oldDesc.strideBytes / sizeof(cmod::VWord);
    const cmod::VWord* oldVertex = oldVertexData + stride * oldIndex;

    for (std::size_t i = 0; i < newDesc.attributes.size(); i++)
    {
        if (fromOffsets[i] != ~0u)
        {
            std::memcpy(newVertexData + newDesc.attributes[i].offsetWords,
                        oldVertex + fromOffsets[i],
                        cmod::VertexAttribute::getFormatSizeWords(newDesc.attributes[i].format) * sizeof(cmod::VWord));
        }
    }
}

Eigen::Vector3f
getVertex(const cmod::VWord* vertexData,
          int positionOffset,
          std::uint32_t strideWords,
          cmod::Index32 index)
{
    std::array<float, 3> fdata;
    std::memcpy(fdata.data(), vertexData + strideWords * index + positionOffset, sizeof(float) * 3);
    return Eigen::Vector3f::Map(fdata.data());
}


Eigen::Vector2f
getTexCoord(const cmod::VWord* vertexData,
            int texCoordOffset,
            std::uint32_t strideWords,
            cmod::Index32 index)
{
    std::array<float, 2> fdata;
    std::memcpy(fdata.data(), vertexData + strideWords * index + texCoordOffset, sizeof(float) * 2);
    return Eigen::Vector2f::Map(fdata.data());
}

Eigen::Vector3f
averageFaceVectors(const std::vector<Face>& faces,
                   std::uint32_t thisFace,
                   const std::uint32_t* vertexFaces,
                   std::uint32_t vertexFaceCount)
{
    const Face& face = faces[thisFace];

    Eigen::Vector3f v = Eigen::Vector3f::Zero();
    for (std::uint32_t i = 0; i < vertexFaceCount; i++)
    {
        std::uint32_t f = vertexFaces[i];
        float cosAngle = face.normal.dot(faces[f].normal);
        if (f == thisFace || cosAngle > 0.0f)
            v += faces[f].normal;
    }

    return v.squaredNorm() == 0.0f ? Eigen::Vector3f::UnitX() : v.normalized();
}

void
augmentVertexDescription(cmod::VertexDescription& desc,
                         cmod::VertexAttributeSemantic semantic,
                         cmod::VertexAttributeFormat format)
{
    std::uint32_t stride = 0;
    bool foundMatch = false;

    auto it = desc.attributes.begin();
    auto end = desc.attributes.end();
    for (auto &attr : desc.attributes)
    {
        if (semantic == attr.semantic && format != attr.format)
        {
            // The semantic matches, but the format does not; skip this
            // item.
            continue;
        }

        foundMatch |= (semantic == attr.semantic);
        attr.offsetWords = stride;
        stride += cmod::VertexAttribute::getFormatSizeWords(attr.format);
        *it = std::move(attr);
        ++it;
    }

    desc.attributes.erase(it, end);

    if (!foundMatch)
    {
        desc.attributes.emplace_back(semantic, format, stride);
        stride += cmod::VertexAttribute::getFormatSizeWords(format);
    }

    desc.strideBytes = stride * sizeof(cmod::VWord);
}

} // namespace

namespace cmod
{

Mesh
GenerateTangents(const Mesh& mesh)
{
    std::uint32_t nVertices = mesh.getVertexCount();

    // In order to generate tangents, we require positions, normals, and
    // 2D texture coordinates in the vertex description.
    const auto& desc = mesh.getVertexDescription();
    if (desc.getAttribute(VertexAttributeSemantic::Position).format != VertexAttributeFormat::Float3)
    {
        GetLogger()->error("Vertex position must be a float3\n");
        return {};
    }

    if (desc.getAttribute(VertexAttributeSemantic::Normal).format != VertexAttributeFormat::Float3)
    {
        GetLogger()->error("float3 format vertex normal required\n");
        return {};
    }

    if (desc.getAttribute(VertexAttributeSemantic::Texture0).format == VertexAttributeFormat::InvalidFormat)
    {
        GetLogger()->error("Texture coordinates must be present in mesh to generate tangents\n");
        return {};
    }

    if (desc.getAttribute(VertexAttributeSemantic::Texture0).format != VertexAttributeFormat::Float2)
    {
        GetLogger()->error("Texture coordinate must be a float2\n");
        return {};
    }

    // Count the number of faces in the mesh.
    // (All geometry should already converted to triangle lists)
    std::uint32_t nFaces = 0;
    for (std::uint32_t i = 0; mesh.getGroup(i) != nullptr; i++)
    {
        const PrimitiveGroup* group = mesh.getGroup(i);
        if (group->prim == PrimitiveGroupType::TriList)
        {
            assert(group->indices.size() % 3 == 0);
            nFaces += group->indices.size() / 3;
        }
        else
        {
            GetLogger()->error("Mesh should contain just triangle lists\n");
            return {};
        }
    }

    // Build the array of faces; this may require decomposing triangle strips
    // and fans into triangle lists.
    std::vector<Face> faces(nFaces);

    for (std::uint32_t i = 0, f = 0; mesh.getGroup(i) != nullptr; i++)
    {
        const PrimitiveGroup* group = mesh.getGroup(i);

        if (group->prim != PrimitiveGroupType::TriList)
            continue;

        for (std::uint32_t j = 0; j < group->indices.size() / 3; j++)
        {
            assert(f < nFaces);
            faces[f].i[0] = group->indices[j * 3];
            faces[f].i[1] = group->indices[j * 3 + 1];
            faces[f].i[2] = group->indices[j * 3 + 2];
            f++;
        }
    }

    unsigned int stride = desc.strideBytes / sizeof(VWord);
    std::uint32_t posOffset = desc.getAttribute(VertexAttributeSemantic::Position).offsetWords;
    std::uint32_t texCoordOffset = desc.getAttribute(VertexAttributeSemantic::Texture0).offsetWords;

    const VWord* vertexData = mesh.getVertexData();

    // Compute tangents for faces
    for (std::uint32_t f = 0; f < nFaces; f++)
    {
        Face& face = faces[f];
        Eigen::Vector3f p0 = getVertex(vertexData, posOffset, stride, face.i[0]);
        Eigen::Vector3f p1 = getVertex(vertexData, posOffset, stride, face.i[1]);
        Eigen::Vector3f p2 = getVertex(vertexData, posOffset, stride, face.i[2]);
        Eigen::Vector2f tc0 = getTexCoord(vertexData, texCoordOffset, stride, face.i[0]);
        Eigen::Vector2f tc1 = getTexCoord(vertexData, texCoordOffset, stride, face.i[1]);
        Eigen::Vector2f tc2 = getTexCoord(vertexData, texCoordOffset, stride, face.i[2]);
        float s1 = tc1.x() - tc0.x();
        float s2 = tc2.x() - tc0.x();
        float t1 = tc1.y() - tc0.y();
        float t2 = tc2.y() - tc0.y();
        float a = s1 * t2 - s2 * t1;
        if (a != 0.0f)
            face.normal = (t2 * (p1 - p0) - t1 * (p2 - p0)) * (1.0f / a);
        else
            face.normal = Eigen::Vector3f::Zero();
    }

    // Count the number of faces in which each vertex appears
    std::vector<std::uint32_t> faceCounts(nVertices, 0);
    for (std::uint32_t f = 0; f < nFaces; f++)
    {
        const Face& face = faces[f];
        faceCounts[face.i[0]]++;
        faceCounts[face.i[1]]++;
        faceCounts[face.i[2]]++;
    }

    // Calculate space ammount for the per-vertex face lists
    std::uint32_t vertexTotal = std::accumulate(faceCounts.begin(), faceCounts.end(), 0u) + nVertices;
    // Use a single buffer for all vertex faces lists
    std::vector<std::uint32_t> vertexFacesData(vertexTotal, 0u);
    // Store pointers to actual vertex faces data lists
    std::vector<std::uint32_t*> vertexFaces(nVertices, nullptr);

    for (std::uint32_t i = 0, offset = 0u; i < nVertices; i++)
    {
        if (faceCounts[i] > 0)
        {
            std::uint32_t count = faceCounts[i] + 1;
            vertexFaces[i] = vertexFacesData.data() + offset;
            vertexFaces[i][0] = faceCounts[i];
            offset += count;
        }
    }

    // Fill in the vertex/face lists
    for (std::uint32_t f = 0; f < nFaces; f++)
    {
        const Face& face = faces[f];
        vertexFaces[face.i[0]][faceCounts[face.i[0]]--] = f;
        vertexFaces[face.i[1]][faceCounts[face.i[1]]--] = f;
        vertexFaces[face.i[2]][faceCounts[face.i[2]]--] = f;
    }

    // Compute the vertex tangents by averaging
    std::vector<Eigen::Vector3f> vertexTangents(nFaces * 3);
    for (std::uint32_t f = 0; f < nFaces; f++)
    {
        const Face& face = faces[f];
        for (std::uint32_t j = 0; j < 3; j++)
        {
            vertexTangents[f * 3 + j] = averageFaceVectors(faces, f,
                                                           &vertexFaces[face.i[j]][1],
                                                           vertexFaces[face.i[j]][0]);
        }
    }

    // Create the new vertex description
    VertexDescription newDesc = desc.clone();
    augmentVertexDescription(newDesc, VertexAttributeSemantic::Tangent, VertexAttributeFormat::Float3);

    // We need to convert the copy the old vertex attributes to the new
    // mesh.  In order to do this, we need the old offset of each attribute
    // in the new vertex description.  The fromOffsets array will contain
    // this mapping.
    std::uint32_t tangentOffset = 0;
    std::array<std::uint32_t, 16> fromOffsets;
    for (std::uint32_t i = 0; i < newDesc.attributes.size(); i++)
    {
        fromOffsets[i] = ~0;

        if (newDesc.attributes[i].semantic == VertexAttributeSemantic::Tangent)
        {
            tangentOffset = newDesc.attributes[i].offsetWords;
        }
        else
        {
            for (const auto& oldAttr : desc.attributes)
            {
                if (oldAttr.semantic == newDesc.attributes[i].semantic)
                {
                    assert(oldAttr.format == newDesc.attributes[i].format);
                    fromOffsets[i] = oldAttr.offsetWords;
                    break;
                }
            }
        }
    }

    // Copy the old vertex data along with the generated tangents to the
    // new vertex data buffer.
    unsigned int newStride = newDesc.strideBytes / sizeof(VWord);
    std::vector<VWord> newVertexData(newStride * nFaces * 3);
    for (std::uint32_t f = 0; f < nFaces; f++)
    {
        const Face& face = faces[f];

        for (std::uint32_t j = 0; j < 3; j++)
        {
            VWord* newVertex = newVertexData.data() + (f * 3 + j) * newStride;
            copyVertex(newVertex, newDesc,
                       vertexData, desc,
                       face.i[j],
                       fromOffsets);
            std::memcpy(newVertex + tangentOffset, vertexTangents[f * 3 + j].data(), 3 * sizeof(float));
        }
    }

    // Create the Celestia mesh
    Mesh newMesh;
    newMesh.setVertexDescription(std::move(newDesc));
    newMesh.setVertices(nFaces * 3, std::move(newVertexData));

    std::uint32_t firstIndex = 0;
    for (std::uint32_t groupIndex = 0; mesh.getGroup(groupIndex) != nullptr; ++groupIndex)
    {
        const PrimitiveGroup* group = mesh.getGroup(groupIndex);
        unsigned int faceCount = 0;

        switch (group->prim)
        {
        case PrimitiveGroupType::TriList:
            faceCount = static_cast<std::uint32_t>(group->indices.size()) / 3u;
            break;
        case PrimitiveGroupType::TriStrip:
        case PrimitiveGroupType::TriFan:
            faceCount = static_cast<std::uint32_t>(group->indices.size()) - 2u;
            break;
        default:
            assert(0);
            break;
        }

        // Create a trivial index list
        std::vector<Index32> indices(faceCount * 3);
        std::iota(indices.begin(), indices.end(), firstIndex);

        newMesh.addGroup(PrimitiveGroupType::TriList,
                         mesh.getGroup(groupIndex)->materialIndex,
                         std::move(indices));
        firstIndex += faceCount * 3;
    }

    return newMesh;
}

} // namespace cmod
