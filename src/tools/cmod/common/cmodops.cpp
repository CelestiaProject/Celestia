// cmodops.cpp
//
// Copyright (C) 2004-2010, Chris Laurel <claurel@gmail.com>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// Perform various adjustments to a Celestia mesh.

#include <algorithm>
#include <array>
#include <cassert>
#include <cmath>
#include <cstring>
#include <iostream>
#include <iterator>
#include <numeric>
#include <utility>
#include <vector>

#include <celmodel/model.h>

#include "cmodops.h"


namespace
{
struct Vertex
{
    Vertex() :
        index(0),
        attributes(nullptr)
    {};

    Vertex(std::uint32_t _index, const cmod::VWord* _attributes) :
        index(_index),
        attributes(_attributes)
    {};

    std::uint32_t index;
    const cmod::VWord* attributes;
};


struct Face
{
    Eigen::Vector3f normal;
    std::uint32_t i[3];    // vertex attribute indices
    std::uint32_t vi[3];   // vertex point indices -- same as above unless welding
};


struct VertexComparator
{
    virtual bool compare(const Vertex& a, const Vertex& b) const = 0;
    virtual ~VertexComparator() = default;

    bool operator()(const Vertex& a, const Vertex& b) const
    {
        return compare(a, b);
    }
};


class FullComparator : public VertexComparator
{
public:
    FullComparator(int _vertexSize) :
        vertexSize(_vertexSize)
    {
    }

    bool compare(const Vertex& a, const Vertex& b) const override
    {
        return std::lexicographical_compare(a.attributes, a.attributes + vertexSize,
                                            b.attributes, b.attributes + vertexSize);
    }

private:
    int vertexSize;
};


class PointOrderingPredicate : public VertexComparator
{
public:
    bool compare(const Vertex& a, const Vertex& b) const override
    {
        std::array<float, 3> p0;
        std::memcpy(p0.data(), a.attributes, sizeof(float) * 3);

        std::array<float, 3> p1;
        std::memcpy(p1.data(), b.attributes, sizeof(float) * 3);

        return p0 < p1;
    }

private:
    int ignore;
};


class PointTexCoordOrderingPredicate : public VertexComparator
{
public:
    PointTexCoordOrderingPredicate(std::uint32_t _posOffset,
                                   std::uint32_t _texCoordOffset,
                                   bool _wrap) :
        posOffset(_posOffset),
        texCoordOffset(_texCoordOffset),
        wrap(_wrap)
    {
    }

    bool compare(const Vertex& a, const Vertex& b) const override
    {
        std::array<float, 5> ptc0;
        std::memcpy(ptc0.data(), a.attributes + posOffset, sizeof(float) * 3);
        std::memcpy(ptc0.data() + 3, a.attributes + texCoordOffset, sizeof(float) * 2);

        std::array<float, 5> ptc1;
        std::memcpy(ptc1.data(), b.attributes + posOffset, sizeof(float) * 3);
        std::memcpy(ptc1.data() + 3, b.attributes + texCoordOffset, sizeof(float) * 2);

        return ptc0 < ptc1;
    }

private:
    std::uint32_t posOffset;
    std::uint32_t texCoordOffset;
    bool wrap;
};


bool approxEqual(float x, float y, float prec)
{
    return std::abs(x - y) <= prec * std::min(std::abs(x), std::abs(y));
}


class PointEquivalencePredicate : public VertexComparator
{
public:
    PointEquivalencePredicate(std::uint32_t _posOffset,
                              float _tolerance) :
        posOffset(_posOffset),
        tolerance(_tolerance)
    {
    }

    bool compare(const Vertex& a, const Vertex& b) const override
    {
        std::array<float, 3> p0;
        std::memcpy(p0.data(), a.attributes + posOffset, sizeof(float) * 3);

        std::array<float, 3> p1;
        std::memcpy(p1.data(), b.attributes + posOffset, sizeof(float) * 3);

        return std::equal(p0.cbegin(), p0.cend(), p1.cbegin(),
                          [&](float f0, float f1) { return approxEqual(f0, f1, tolerance); });
    }

private:
    std::uint32_t posOffset;
    float tolerance;
};


class PointTexCoordEquivalencePredicate : public VertexComparator
{
public:
    PointTexCoordEquivalencePredicate(std::uint32_t _posOffset,
                                      std::uint32_t _texCoordOffset,
                                      bool _wrap,
                                      float _tolerance) :
        posOffset(_posOffset),
        texCoordOffset(_texCoordOffset),
        wrap(_wrap),
        tolerance(_tolerance)
    {
    }

    bool compare(const Vertex& a, const Vertex& b) const override
    {
        std::array<float, 5> ptc0;
        std::memcpy(ptc0.data(), a.attributes + posOffset, sizeof(float) * 3);
        std::memcpy(ptc0.data() + 3, a.attributes + texCoordOffset, sizeof(float) * 2);

        std::array<float, 5> ptc1;
        std::memcpy(ptc1.data(), b.attributes + posOffset, sizeof(float) * 3);
        std::memcpy(ptc1.data() + 3, b.attributes + texCoordOffset, sizeof(float) * 2);

        return std::equal(ptc0.cbegin(), ptc0.cend(), ptc1.cbegin(),
                          [&](float f0, float f1) { return approxEqual(f0, f1, tolerance); });
    }

private:
    std::uint32_t posOffset;
    std::uint32_t texCoordOffset;
    bool wrap;
    float tolerance;
};


bool equal(const Vertex& a, const Vertex& b, std::uint32_t vertexSize)
{
    return std::equal(a.attributes, a.attributes + vertexSize, b.attributes);
}


bool equalPoint(const Vertex& a, const Vertex& b)
{
    std::array<float, 3> p0;
    std::memcpy(p0.data(), a.attributes, sizeof(float) * 3);

    std::array<float, 3> p1;
    std::memcpy(p1.data(), b.attributes, sizeof(float) * 3);

    return p0 == p1;
}


Eigen::Vector3f
getVertex(const cmod::VWord* vertexData,
          int positionOffset,
          std::uint32_t strideWords,
          std::uint32_t index)
{
    float fdata[3];
    std::memcpy(fdata, vertexData + strideWords * index + positionOffset, sizeof(float) * 3);
    return Eigen::Vector3f(fdata);
}


Eigen::Vector2f
getTexCoord(const cmod::VWord* vertexData,
            int texCoordOffset,
            uint32_t strideWords,
            uint32_t index)
{
    float fdata[2];
    std::memcpy(fdata, vertexData + strideWords * index + texCoordOffset, sizeof(float) * 2);
    return Eigen::Vector2f(fdata);
}


Eigen::Vector3f
averageFaceVectors(const std::vector<Face>& faces,
                   std::uint32_t thisFace,
                   std::uint32_t* vertexFaces,
                   std::uint32_t vertexFaceCount,
                   float cosSmoothingAngle)
{
    const Face& face = faces[thisFace];

    Eigen::Vector3f v = Eigen::Vector3f::Zero();
    for (std::uint32_t i = 0; i < vertexFaceCount; i++)
    {
        std::uint32_t f = vertexFaces[i];
        float cosAngle = face.normal.dot(faces[f].normal);
        if (f == thisFace || cosAngle > cosSmoothingAngle)
            v += faces[f].normal;
    }

    if (v.squaredNorm() == 0.0f)
        v = Eigen::Vector3f::UnitX();
    else
        v.normalize();

    return v;
}


void
copyVertex(cmod::VWord* newVertexData,
           const cmod::VertexDescription& newDesc,
           const cmod::VWord* oldVertexData,
           const cmod::VertexDescription& oldDesc,
           std::uint32_t oldIndex,
           const std::uint32_t fromOffsets[])
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


void
augmentVertexDescription(cmod::VertexDescription& desc,
                         cmod::VertexAttributeSemantic semantic,
                         cmod::VertexAttributeFormat format)
{
    std::uint32_t stride = 0;
    bool foundMatch = false;

    auto it = desc.attributes.begin();
    auto end = desc.attributes.end();
    for (auto i = desc.attributes.begin(); i != end; ++i)
    {
        if (semantic == i->semantic && format != i->format)
        {
            // The semantic matches, but the format does not; skip this
            // item.
            continue;
        }

        foundMatch |= (semantic == i->semantic);
        i->offsetWords = stride;
        stride += cmod::VertexAttribute::getFormatSizeWords(i->format);
        *it++ = std::move(*i);
    }

    desc.attributes.erase(it, end);

    if (!foundMatch)
    {
        desc.attributes.emplace_back(semantic, format, stride);
        stride += cmod::VertexAttribute::getFormatSizeWords(format);
    }

    desc.strideBytes = stride * sizeof(cmod::VWord);
}


void
addGroupWithOffset(cmod::Mesh& mesh,
                   const cmod::PrimitiveGroup& group,
                   std::uint32_t offset)
{
    if (group.indices.empty())
        return;

    std::vector<cmod::Index32> newIndices;
    newIndices.reserve(group.indices.size());
    std::transform(group.indices.cbegin(), group.indices.cend(),
                   std::back_inserter(newIndices),
                   [=](cmod::Index32 idx) { return idx + offset; });

    mesh.addGroup(group.prim, group.materialIndex, std::move(newIndices));
}


template<typename T, typename U> void
joinVertices(std::vector<Face>& faces,
             const cmod::VWord* vertexData,
             const cmod::VertexDescription& desc,
             const T& orderingPredicate,
             const U& equivalencePredicate)
{
    // Don't do anything if we're given no data
    if (faces.size() == 0)
        return;

    // Must have a position
    assert(desc.getAttribute(cmod::VertexAttributeSemantic::Position).format == cmod::VertexAttributeFormat::Float3);

    std::uint32_t posOffset = desc.getAttribute(cmod::VertexAttributeSemantic::Position).offsetWords;
    const cmod::VWord* vertexPoints = vertexData + posOffset;
    std::uint32_t nVertices = faces.size() * 3;

    // Initialize the array of vertices
    std::vector<Vertex> vertices(nVertices);
    std::uint32_t f;
    unsigned int stride = desc.strideBytes / sizeof(cmod::VWord);
    for (f = 0; f < faces.size(); f++)
    {
        for (std::uint32_t j = 0; j < 3; j++)
        {
            std::uint32_t index = faces[f].i[j];
            vertices[f * 3 + j] = Vertex(index, vertexPoints + stride * index);

        }
    }

    // Sort the vertices so that identical ones will be ordered consecutively
    std::sort(vertices.begin(), vertices.end(), orderingPredicate);

    // Build the vertex merge map
    std::vector<std::uint32_t> mergeMap(nVertices);
    std::uint32_t lastUnique = 0;
    std::uint32_t uniqueCount = 0;
    for (std::uint32_t i = 0; i < nVertices; i++)
    {
        if (i == 0 || !equivalencePredicate(vertices[i - 1], vertices[i]))
        {
            lastUnique = i;
            uniqueCount++;
        }

        mergeMap[vertices[i].index] = vertices[lastUnique].index;
    }

    // Remap the vertex indices
    for (f = 0; f < faces.size(); f++)
    {
        for (std::uint32_t k= 0; k < 3; k++)
        {
            faces[f].vi[k] = mergeMap[faces[f].i[k]];
        }
    }
}


} // end unnamed namespace


/** Generate surface normals for a mesh. A new mesh with normals is returned, and
  * the original mesh is unmodified.
  *
  * @param mesh the mesh to generate normals for
  * @param smoothAngle maximum angle (in radians) between two faces that are
  *                    treated as belonging to the same smooth surface patch
  * @param weld true if vertices with identical positions should be treated
  *             as the same during normal generation (typically should be true)
  * @param weldTolerance maximum difference between positions that should be
  *             considered identical during the weld step.
  */
cmod::Mesh
GenerateNormals(const cmod::Mesh& mesh, float smoothAngle, bool weld, float weldTolerance)
{
    std::uint32_t nVertices = mesh.getVertexCount();
    float cosSmoothAngle = std::cos(smoothAngle);

    const cmod::VertexDescription& desc = mesh.getVertexDescription();

    if (desc.getAttribute(cmod::VertexAttributeSemantic::Position).format != cmod::VertexAttributeFormat::Float3)
    {
        std::cerr << "Vertex position must be a float3\n";
        return cmod::Mesh();
    }

    std::uint32_t posOffset = desc.getAttribute(cmod::VertexAttributeSemantic::Position).offsetWords;
    unsigned int stride = desc.strideBytes / sizeof(cmod::VWord);

    std::uint32_t nFaces = 0;
    std::uint32_t i;
    for (i = 0; mesh.getGroup(i) != nullptr; i++)
    {
        const cmod::PrimitiveGroup* group = mesh.getGroup(i);

        switch (group->prim)
        {
        case cmod::PrimitiveGroupType::TriList:
            if (group->indices.size() < 3 || group->indices.size() % 3 != 0)
            {
                std::cerr << "Triangle list has invalid number of indices\n";
                return cmod::Mesh();
            }
            nFaces += group->indices.size() / 3;
            break;

        case cmod::PrimitiveGroupType::TriStrip:
        case cmod::PrimitiveGroupType::TriFan:
            if (group->indices.size() < 3)
            {
                std::cerr << "Error: tri strip or fan has less than three indices\n";
                return cmod::Mesh();
            }
            nFaces += group->indices.size() - 2;
            break;

        default:
            std::cerr << "Cannot generate normals for non-triangle primitives\n";
            return cmod::Mesh();
        }
    }

    // Build the array of faces; this may require decomposing triangle strips
    // and fans into triangle lists.
    std::vector<Face> faces(nFaces);

    std::uint32_t f = 0;
    for (i = 0; mesh.getGroup(i) != nullptr; i++)
    {
        const cmod::PrimitiveGroup* group = mesh.getGroup(i);

        switch (group->prim)
        {
        case cmod::PrimitiveGroupType::TriList:
            {
                for (std::uint32_t j = 0; j < group->indices.size() / 3; j++)
                {
                    assert(f < nFaces);
                    faces[f].i[0] = group->indices[j * 3];
                    faces[f].i[1] = group->indices[j * 3 + 1];
                    faces[f].i[2] = group->indices[j * 3 + 2];
                    f++;
                }
            }
            break;

        case cmod::PrimitiveGroupType::TriStrip:
            {
                for (std::uint32_t j = 2; j < group->indices.size(); j++)
                {
                    assert(f < nFaces);
                    if (j % 2 == 0)
                    {
                        faces[f].i[0] = group->indices[j - 2];
                        faces[f].i[1] = group->indices[j - 1];
                        faces[f].i[2] = group->indices[j];
                    }
                    else
                    {
                        faces[f].i[0] = group->indices[j - 1];
                        faces[f].i[1] = group->indices[j - 2];
                        faces[f].i[2] = group->indices[j];
                    }
                    f++;
                }
            }
            break;

        case cmod::PrimitiveGroupType::TriFan:
            {
                for (std::uint32_t j = 2; j < group->indices.size(); j++)
                {
                    assert(f < nFaces);
                    faces[f].i[0] = group->indices[0];
                    faces[f].i[1] = group->indices[j - 1];
                    faces[f].i[2] = group->indices[j];
                    f++;
                }
            }
            break;

        default:
            assert(0);
            break;
        }
    }
    assert(f == nFaces);

    const cmod::VWord* vertexData = mesh.getVertexData();

    // Compute normals for the faces
    for (f = 0; f < nFaces; f++)
    {
        Face& face = faces[f];
        Eigen::Vector3f p0 = getVertex(vertexData, posOffset, stride, face.i[0]);
        Eigen::Vector3f p1 = getVertex(vertexData, posOffset, stride, face.i[1]);
        Eigen::Vector3f p2 = getVertex(vertexData, posOffset, stride, face.i[2]);
        face.normal = (p1 - p0).cross(p2 - p1);
        if (face.normal.squaredNorm() > 0.0f)
        {
            face.normal.normalize();
        }
    }

    // For each vertex, create a list of faces that contain it
    std::vector<std::uint32_t> faceCounts(nVertices, 0);
    std::vector<std::vector<std::uint32_t>> vertexFaces(nVertices);

    // If we're welding vertices before generating normals, find identical
    // points and merge them.  Otherwise, the point indices will be the same
    // as the attribute indices.
    if (weld)
    {
        joinVertices(faces, vertexData, desc,
                     PointOrderingPredicate(),
                     PointEquivalencePredicate(0, weldTolerance));
    }
    else
    {
        for (f = 0; f < nFaces; f++)
        {
            faces[f].vi[0] = faces[f].i[0];
            faces[f].vi[1] = faces[f].i[1];
            faces[f].vi[2] = faces[f].i[2];
        }
    }

    // Count the number of faces in which each vertex appears
    for (f = 0; f < nFaces; f++)
    {
        Face& face = faces[f];
        faceCounts[face.vi[0]]++;
        faceCounts[face.vi[1]]++;
        faceCounts[face.vi[2]]++;
    }

    // Allocate space for the per-vertex face lists
    for (i = 0; i < nVertices; i++)
    {
        if (faceCounts[i] > 0)
        {
            vertexFaces[i].resize(faceCounts[i] + 1);
            vertexFaces[i][0] = faceCounts[i];
        }
    }

    // Fill in the vertex/face lists
    for (f = 0; f < nFaces; f++)
    {
        Face& face = faces[f];
        vertexFaces[face.vi[0]][faceCounts[face.vi[0]]--] = f;
        vertexFaces[face.vi[1]][faceCounts[face.vi[1]]--] = f;
        vertexFaces[face.vi[2]][faceCounts[face.vi[2]]--] = f;
    }

    // Compute the vertex normals by averaging
    std::vector<Eigen::Vector3f> vertexNormals(nFaces * 3);
    for (f = 0; f < nFaces; f++)
    {
        Face& face = faces[f];
        for (std::uint32_t j = 0; j < 3; j++)
        {
            vertexNormals[f * 3 + j] =
                averageFaceVectors(faces, f,
                                   &vertexFaces[face.vi[j]][1],
                                   vertexFaces[face.vi[j]][0],
                                   cosSmoothAngle);
        }
    }

    // Finally, create a new mesh with normals included

    // Create the new vertex description
    cmod::VertexDescription newDesc = desc.clone();
    augmentVertexDescription(newDesc, cmod::VertexAttributeSemantic::Normal, cmod::VertexAttributeFormat::Float3);

    // We need to convert the copy the old vertex attributes to the new
    // mesh.  In order to do this, we need the old offset of each attribute
    // in the new vertex description.  The fromOffsets array will contain
    // this mapping.
    std::uint32_t normalOffset = 0;
    std::uint32_t fromOffsets[16];
    for (i = 0; i < newDesc.attributes.size(); i++)
    {
        fromOffsets[i] = ~0;

        if (newDesc.attributes[i].semantic == cmod::VertexAttributeSemantic::Normal)
        {
            normalOffset = newDesc.attributes[i].offsetWords;
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

    // Copy the old vertex data along with the generated normals to the
    // new vertex data buffer.
    unsigned int newStride = newDesc.strideBytes / sizeof(cmod::VWord);
    std::vector<cmod::VWord> newVertexData(newStride * nFaces * 3);
    for (f = 0; f < nFaces; f++)
    {
        Face& face = faces[f];

        for (std::uint32_t j = 0; j < 3; j++)
        {
            cmod::VWord* newVertex = newVertexData.data() + (f * 3 + j) * newStride;
            copyVertex(newVertex, newDesc,
                       vertexData, desc,
                       face.i[j],
                       fromOffsets);
            std::memcpy(newVertex + normalOffset, &vertexNormals[f * 3 + j],
                        cmod::VertexAttribute::getFormatSizeWords(cmod::VertexAttributeFormat::Float3) * sizeof(cmod::VWord));
        }
    }

    // Create the Celestia mesh
    cmod::Mesh newMesh;
    newMesh.setVertexDescription(std::move(newDesc));
    newMesh.setVertices(nFaces * 3, std::move(newVertexData));

    std::uint32_t firstIndex = 0;
    for (std::uint32_t groupIndex = 0; mesh.getGroup(groupIndex) != 0; ++groupIndex)
    {
        const cmod::PrimitiveGroup* group = mesh.getGroup(groupIndex);
        unsigned int faceCount = 0;

        switch (group->prim)
        {
        case cmod::PrimitiveGroupType::TriList:
            faceCount = group->indices.size() / 3;
            break;
        case cmod::PrimitiveGroupType::TriStrip:
        case cmod::PrimitiveGroupType::TriFan:
            faceCount = group->indices.size() - 2;
            break;
        default:
            assert(0);
            break;
        }

        // Create a trivial index list
        std::vector<cmod::Index32> indices(faceCount * 3);
        std::iota(indices.begin(), indices.end(), firstIndex);

        newMesh.addGroup(cmod::PrimitiveGroupType::TriList,
                         mesh.getGroup(groupIndex)->materialIndex,
                         std::move(indices));
        firstIndex += faceCount * 3;
    }

    return newMesh;
}


cmod::Mesh
GenerateTangents(const cmod::Mesh& mesh, bool weld)
{
    uint32_t nVertices = mesh.getVertexCount();

    // In order to generate tangents, we require positions, normals, and
    // 2D texture coordinates in the vertex description.
    const cmod::VertexDescription& desc = mesh.getVertexDescription();
    if (desc.getAttribute(cmod::VertexAttributeSemantic::Position).format != cmod::VertexAttributeFormat::Float3)
    {
        std::cerr << "Vertex position must be a float3\n";
        return cmod::Mesh();
    }

    if (desc.getAttribute(cmod::VertexAttributeSemantic::Normal).format != cmod::VertexAttributeFormat::Float3)
    {
        std::cerr << "float3 format vertex normal required\n";
        return cmod::Mesh();
    }

    if (desc.getAttribute(cmod::VertexAttributeSemantic::Texture0).format == cmod::VertexAttributeFormat::InvalidFormat)
    {
        std::cerr << "Texture coordinates must be present in mesh to generate tangents\n";
        return cmod::Mesh();
    }

    if (desc.getAttribute(cmod::VertexAttributeSemantic::Texture0).format != cmod::VertexAttributeFormat::Float2)
    {
        std::cerr << "Texture coordinate must be a float2\n";
        return cmod::Mesh();
    }

    // Count the number of faces in the mesh.
    // (All geometry should already converted to triangle lists)
    std::uint32_t i;
    std::uint32_t nFaces = 0;
    for (i = 0; mesh.getGroup(i) != nullptr; i++)
    {
        const cmod::PrimitiveGroup* group = mesh.getGroup(i);
        if (group->prim == cmod::PrimitiveGroupType::TriList)
        {
            assert(group->indices.size() % 3 == 0);
            nFaces += group->indices.size() / 3;
        }
        else
        {
            std::cerr << "Mesh should contain just triangle lists\n";
            return cmod::Mesh();
        }
    }

    // Build the array of faces; this may require decomposing triangle strips
    // and fans into triangle lists.
    std::vector<Face> faces(nFaces);

    std::uint32_t f = 0;
    for (i = 0; mesh.getGroup(i) != nullptr; i++)
    {
        const cmod::PrimitiveGroup* group = mesh.getGroup(i);

        switch (group->prim)
        {
        case cmod::PrimitiveGroupType::TriList:
            {
                for (uint32_t j = 0; j < group->indices.size() / 3; j++)
                {
                    assert(f < nFaces);
                    faces[f].i[0] = group->indices[j * 3];
                    faces[f].i[1] = group->indices[j * 3 + 1];
                    faces[f].i[2] = group->indices[j * 3 + 2];
                    f++;
                }
            }
            break;

        default:
            break;
        }
    }

    unsigned int stride = desc.strideBytes / sizeof(cmod::VWord);
    std::uint32_t posOffset = desc.getAttribute(cmod::VertexAttributeSemantic::Position).offsetWords;
    std::uint32_t texCoordOffset = desc.getAttribute(cmod::VertexAttributeSemantic::Texture0).offsetWords;

    const cmod::VWord* vertexData = mesh.getVertexData();

    // Compute tangents for faces
    for (f = 0; f < nFaces; f++)
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

    // For each vertex, create a list of faces that contain it
    std::uint32_t* faceCounts = new std::uint32_t[nVertices];
    std::uint32_t** vertexFaces = new std::uint32_t*[nVertices];

    // Initialize the lists
    for (i = 0; i < nVertices; i++)
    {
        faceCounts[i] = 0;
        vertexFaces[i] = nullptr;
    }

    // If we're welding vertices before generating normals, find identical
    // points and merge them.  Otherwise, the point indices will be the same
    // as the attribute indices.
    if (weld)
    {
        joinVertices(faces, vertexData, desc,
                     PointTexCoordOrderingPredicate(posOffset, texCoordOffset, true),
                     PointTexCoordEquivalencePredicate(posOffset, texCoordOffset, true, 1.0e-5f));
    }
    else
    {
        for (f = 0; f < nFaces; f++)
        {
            faces[f].vi[0] = faces[f].i[0];
            faces[f].vi[1] = faces[f].i[1];
            faces[f].vi[2] = faces[f].i[2];
        }
    }

    // Count the number of faces in which each vertex appears
    for (f = 0; f < nFaces; f++)
    {
        Face& face = faces[f];
        faceCounts[face.vi[0]]++;
        faceCounts[face.vi[1]]++;
        faceCounts[face.vi[2]]++;
    }

    // Allocate space for the per-vertex face lists
    for (i = 0; i < nVertices; i++)
    {
        if (faceCounts[i] > 0)
        {
            vertexFaces[i] = new std::uint32_t[faceCounts[i] + 1];
            vertexFaces[i][0] = faceCounts[i];
        }
    }

    // Fill in the vertex/face lists
    for (f = 0; f < nFaces; f++)
    {
        Face& face = faces[f];
        vertexFaces[face.vi[0]][faceCounts[face.vi[0]]--] = f;
        vertexFaces[face.vi[1]][faceCounts[face.vi[1]]--] = f;
        vertexFaces[face.vi[2]][faceCounts[face.vi[2]]--] = f;
    }

    // Compute the vertex tangents by averaging
    std::vector<Eigen::Vector3f> vertexTangents(nFaces * 3);
    for (f = 0; f < nFaces; f++)
    {
        Face& face = faces[f];
        for (std::uint32_t j = 0; j < 3; j++)
        {
            vertexTangents[f * 3 + j] =
                averageFaceVectors(faces, f,
                                   &vertexFaces[face.vi[j]][1],
                                   vertexFaces[face.vi[j]][0],
                                   0.0f);
        }
    }

    // Create the new vertex description
    cmod::VertexDescription newDesc = desc.clone();
    augmentVertexDescription(newDesc, cmod::VertexAttributeSemantic::Tangent, cmod::VertexAttributeFormat::Float3);

    // We need to convert the copy the old vertex attributes to the new
    // mesh.  In order to do this, we need the old offset of each attribute
    // in the new vertex description.  The fromOffsets array will contain
    // this mapping.
    std::uint32_t tangentOffset = 0;
    std::uint32_t fromOffsets[16];
    for (i = 0; i < newDesc.attributes.size(); i++)
    {
        fromOffsets[i] = ~0;

        if (newDesc.attributes[i].semantic == cmod::VertexAttributeSemantic::Tangent)
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
    unsigned int newStride = newDesc.strideBytes / sizeof(cmod::VWord);
    std::vector<cmod::VWord> newVertexData(newStride * nFaces * 3);
    for (f = 0; f < nFaces; f++)
    {
        Face& face = faces[f];

        for (std::uint32_t j = 0; j < 3; j++)
        {
            cmod::VWord* newVertex = newVertexData.data() + (f * 3 + j) * newStride;
            copyVertex(newVertex, newDesc,
                       vertexData, desc,
                       face.i[j],
                       fromOffsets);
            std::memcpy(newVertex + tangentOffset, &vertexTangents[f * 3 + j], 3 * sizeof(float));
        }
    }

    // Create the Celestia mesh
    cmod::Mesh newMesh;
    newMesh.setVertexDescription(std::move(newDesc));
    newMesh.setVertices(nFaces * 3, std::move(newVertexData));

    std::uint32_t firstIndex = 0;
    for (std::uint32_t groupIndex = 0; mesh.getGroup(groupIndex) != 0; ++groupIndex)
    {
        const cmod::PrimitiveGroup* group = mesh.getGroup(groupIndex);
        unsigned int faceCount = 0;

        switch (group->prim)
        {
        case cmod::PrimitiveGroupType::TriList:
            faceCount = group->indices.size() / 3;
            break;
        case cmod::PrimitiveGroupType::TriStrip:
        case cmod::PrimitiveGroupType::TriFan:
            faceCount = group->indices.size() - 2;
            break;
        default:
            assert(0);
            break;
        }

        // Create a trivial index list
        std::vector<cmod::Index32> indices(faceCount * 3);
        std::iota(indices.begin(), indices.end(), firstIndex);

        newMesh.addGroup(cmod::PrimitiveGroupType::TriList,
                         mesh.getGroup(groupIndex)->materialIndex,
                         std::move(indices));
        firstIndex += faceCount * 3;
    }

    // Clean up
    delete[] faceCounts;
    for (i = 0; i < nVertices; i++)
    {
        if (vertexFaces[i] != nullptr)
            delete[] vertexFaces[i];
    }
    delete[] vertexFaces;

    return newMesh;
}


bool
UniquifyVertices(cmod::Mesh& mesh)
{
    std::uint32_t nVertices = mesh.getVertexCount();
    const cmod::VertexDescription& desc = mesh.getVertexDescription();

    if (nVertices == 0)
        return false;

    const cmod::VWord* vertexData = mesh.getVertexData();
    if (vertexData == nullptr)
        return false;

    // Initialize the array of vertices
    unsigned int stride = desc.strideBytes / sizeof(cmod::VWord);
    std::vector<Vertex> vertices(nVertices);
    std::uint32_t i;
    for (i = 0; i < nVertices; i++)
    {
        vertices[i] = Vertex(i, vertexData + i * stride);
    }

    // Sort the vertices so that identical ones will be ordered consecutively
    std::sort(vertices.begin(), vertices.end(), FullComparator(stride));

    // Count the number of unique vertices
    std::uint32_t uniqueVertexCount = 0;
    for (i = 0; i < nVertices; i++)
    {
        if (i == 0 || !equal(vertices[i - 1], vertices[i], stride))
            uniqueVertexCount++;
    }

    // No work left to do if we couldn't eliminate any vertices
    if (uniqueVertexCount == nVertices)
        return true;

    // Build the vertex map and the uniquified vertex data
    std::vector<std::uint32_t> vertexMap(nVertices);
    std::vector<cmod::VWord> newVertexData(uniqueVertexCount * stride);
    const cmod::VWord* oldVertexData = mesh.getVertexData();
    std::uint32_t j = 0;
    for (i = 0; i < nVertices; i++)
    {
        if (i == 0 || !equal(vertices[i - 1], vertices[i], stride))
        {
            if (i != 0)
                j++;
            assert(j < uniqueVertexCount);
            std::memcpy(newVertexData.data() + j * stride,
                        oldVertexData + vertices[i].index * stride,
                        desc.strideBytes);
        }
        vertexMap[vertices[i].index] = j;
    }

    // Replace the vertex data with the compacted data
    mesh.setVertices(uniqueVertexCount, std::move(newVertexData));

    mesh.remapIndices(vertexMap);

    return true;
}


// Merge all meshes that share the same vertex description
std::unique_ptr<cmod::Model>
MergeModelMeshes(const cmod::Model& model)
{
    std::vector<const cmod::Mesh*> meshes;

    for (std::uint32_t i = 0; model.getMesh(i) != nullptr; i++)
    {
        meshes.push_back(model.getMesh(i));
    }

    // Sort the meshes by vertex description
    std::sort(meshes.begin(), meshes.end(),
              [](const cmod::Mesh* a, const cmod::Mesh* b) { return a->getVertexDescription() < b->getVertexDescription(); });

    auto newModel = std::make_unique<cmod::Model>();

    // Copy materials into the new model
    for (std::uint32_t i = 0; model.getMaterial(i) != nullptr; i++)
    {
        newModel->addMaterial(model.getMaterial(i)->clone());
    }

    std::uint32_t meshIndex = 0;
    while (meshIndex < meshes.size())
    {
        const cmod::VertexDescription& desc = meshes[meshIndex]->getVertexDescription();

        // Count the number of matching meshes
        std::uint32_t nMatchingMeshes;
        for (nMatchingMeshes = 1;
             meshIndex + nMatchingMeshes < meshes.size();
             nMatchingMeshes++)
        {
            if (!(meshes[meshIndex + nMatchingMeshes]->getVertexDescription() == desc))
            {
                break;
            }
        }

        // Count the number of vertices in all matching meshes
        std::uint32_t totalVertices = 0;
        std::uint32_t j;
        for (j = meshIndex; j < meshIndex + nMatchingMeshes; j++)
        {
            totalVertices += meshes[j]->getVertexCount();
        }

        unsigned int stride = desc.strideBytes / sizeof(cmod::VWord);
        std::vector<cmod::VWord> vertexData(totalVertices * stride);

        // Copy the vertex data
        std::uint32_t vertexCount = 0;
        for (j = meshIndex; j < meshIndex + nMatchingMeshes; ++j)
        {
            const cmod::Mesh* mesh = meshes[j];
            std::memcpy(vertexData.data() + vertexCount * stride,
                        mesh->getVertexData(),
                        mesh->getVertexCount() * desc.strideBytes);
            vertexCount += mesh->getVertexCount();
        }

        // Create the new empty mesh
        cmod::Mesh mergedMesh;
        mergedMesh.setVertexDescription(desc.clone());
        mergedMesh.setVertices(totalVertices, std::move(vertexData));

        // Reindex and add primitive groups
        vertexCount = 0;
        for (j = meshIndex; j < meshIndex + nMatchingMeshes; j++)
        {
            const cmod::Mesh* mesh = meshes[j];
            for (std::uint32_t k = 0; mesh->getGroup(k) != nullptr; k++)
            {
                addGroupWithOffset(mergedMesh, *mesh->getGroup(k),
                                   vertexCount);
            }

            vertexCount += mesh->getVertexCount();
        }
        assert(vertexCount == totalVertices);

        newModel->addMesh(std::move(mergedMesh));

        meshIndex += nMatchingMeshes;
    }

    return newModel;
}


/*! Generate normals for an entire model. Return the new model, or null if
 *  normal generation failed due to an out of memory error.
 */
std::unique_ptr<cmod::Model>
GenerateModelNormals(const cmod::Model& model, float smoothAngle, bool weldVertices, float weldTolerance)
{
    auto newModel = std::make_unique<cmod::Model>();

    // Copy materials
    for (unsigned int i = 0; model.getMaterial(i) != nullptr; i++)
    {
        newModel->addMaterial(model.getMaterial(i)->clone());
    }

    bool ok = true;
    for (unsigned int i = 0; model.getMesh(i) != nullptr; i++)
    {
        const cmod::Mesh* mesh = model.getMesh(i);
        cmod::Mesh newMesh = GenerateNormals(*mesh, smoothAngle, weldVertices, weldTolerance);
        if (newMesh.getVertexCount() == 0)
        {
            ok = false;
        }
        else
        {
            newModel->addMesh(std::move(newMesh));
        }
    }

    if (!ok)
    {
        // If all of the meshes weren't processed due to an out of memory error,
        // delete the new model and return nullptr rather than a partially processed
        // model.
        return nullptr;
    }

    return newModel;
}


#ifdef TRISTRIP
bool
ConvertToStrips(cmod::Mesh& mesh)
{
    std::vector<Mesh::PrimitiveGroup*> groups;

    // NvTriStrip library can only handle 16-bit indices
    if (mesh.getVertexCount() >= 0x10000)
    {
        return true;
    }

    // Verify that the mesh contains just tri strips
    std::uint32_t i;
    for (i = 0; mesh.getGroup(i) != nullptr; i++)
    {
        if (mesh.getGroup(i)->prim != cmod::Mesh::TriList)
            return true;
    }

    // Convert the existing groups to triangle strips
    for (i = 0; mesh.getGroup(i) != nullptr; i++)
    {
        const cmod::Mesh::PrimitiveGroup* group = mesh.getGroup(i);

        // Convert the vertex indices to shorts for the TriStrip library
        unsigned short* indices = new unsigned short[group->nIndices];
        std::uint32_t j;
        for (j = 0; j < group->nIndices; j++)
        {
            indices[j] = (unsigned short) group->indices[j];
        }

        PrimitiveGroup* strips = nullptr;
        unsigned short nGroups;
        bool r = GenerateStrips(indices,
                                group->nIndices,
                                &strips,
                                &nGroups,
                                false);
        if (!r || strips == nullptr)
        {
            std::cerr << "Generate tri strips failed\n";
            return false;
        }

        // Call the tristrip library to convert the lists to strips.  Then,
        // convert from the NvTriStrip's primitive group structure to the
        // CMOD one and add it to the collection that will be added once
        // the mesh's original primitive groups are cleared.
        for (j = 0; j < nGroups; j++)
        {
            cmod::Mesh::PrimitiveGroupType prim = cmod::Mesh::InvalidPrimitiveGroupType;
            switch (strips[j].type)
            {
            case PT_LIST:
                prim = cmod::Mesh::TriList;
                break;
            case PT_STRIP:
                prim = cmod::Mesh::TriStrip;
                break;
            case PT_FAN:
                prim = cmod::Mesh::TriFan;
                break;
            }

            if (prim != cmod::Mesh::InvalidPrimitiveGroupType &&
                strips[j].numIndices != 0)
            {
                cmod::Mesh::PrimitiveGroup* newGroup = new cmod::Mesh::PrimitiveGroup();
                newGroup->prim = prim;
                newGroup->materialIndex = group->materialIndex;
                newGroup->nIndices = strips[j].numIndices;
                newGroup->indices = new uint32_t[newGroup->nIndices];
                for (uint32_t k = 0; k < newGroup->nIndices; k++)
                    newGroup->indices[k] = strips[j].indices[k];

                groups.push_back(newGroup);
            }
        }

        delete[] strips;
    }

    mesh.clearGroups();

    // Add the stripified groups to the mesh
    for (auto iter = groups.begin(); iter != groups.end(); iter++)
    {
        mesh.addGroup(*iter);
    }

    return true;
}
#endif
