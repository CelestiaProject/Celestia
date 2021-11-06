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

#include "cmodops.h"
#include <celmodel/modelfile.h>
#include <celmath/mathlib.h>
#include <Eigen/Core>
#include <algorithm>
#include <iostream>
#include <vector>
#ifdef TRISTRIP
#include <NvTriStrip.h>
#endif

using namespace cmod;
using namespace Eigen;
using namespace std;


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
        const auto* s0 = reinterpret_cast<const char*>(a.attributes);
        const auto* s1 = reinterpret_cast<const char*>(b.attributes);
        for (int i = 0; i < vertexSize; i++)
        {
            if (s0[i] < s1[i])
                return true;
            if (s0[i] > s1[i])
                return false;
        }

        return false;
    }

private:
    int vertexSize;
};


class PointOrderingPredicate : public VertexComparator
{
public:
    bool compare(const Vertex& a, const Vertex& b) const override
    {
        const Vector3f* p0 = reinterpret_cast<const Vector3f*>(a.attributes);
        const Vector3f* p1 = reinterpret_cast<const Vector3f*>(b.attributes);

        if (p0->x() < p1->x())
        {
            return true;
        }
        if (p0->x() > p1->x())
        {
            return false;
        }
        else
        {
            if (p0->y() < p1->y())
                return true;
            if (p0->y() > p1->y())
                return false;
            else
                return p0->z() < p1->z();
        }
    }

private:
    int ignore;
};


class PointTexCoordOrderingPredicate : public VertexComparator
{
public:
    PointTexCoordOrderingPredicate(uint32_t _posOffset,
                                   uint32_t _texCoordOffset,
                                   bool _wrap) :
        posOffset(_posOffset),
        texCoordOffset(_texCoordOffset),
        wrap(_wrap)
    {
    }

    bool compare(const Vertex& a, const Vertex& b) const override
    {
        const auto* adata = reinterpret_cast<const char*>(a.attributes);
        const auto* bdata = reinterpret_cast<const char*>(b.attributes);
        const Vector3f* p0 = reinterpret_cast<const Vector3f*>(adata + posOffset);
        const Vector3f* p1 = reinterpret_cast<const Vector3f*>(bdata + posOffset);
        const Vector2f* tc0 = reinterpret_cast<const Vector2f*>(adata + texCoordOffset);
        const Vector2f* tc1 = reinterpret_cast<const Vector2f*>(bdata + texCoordOffset);
        if (p0->x() < p1->x())
        {
            return true;
        }
        if (p0->x() > p1->x())
        {
            return false;
        }
        else
        {
            if (p0->y() < p1->y())
            {
                return true;
            }
            if (p0->y() > p1->y())
            {
                return false;
            }
            else
            {
                if (p0->z() < p1->z())
                {
                    return true;
                }
                if (p0->z() > p1->z())
                {
                    return false;
                }
                else
                {
                    if (tc0->x() < tc1->x())
                        return true;
                    if (tc0->x() > tc1->x())
                        return false;
                    else
                        return tc0->y() < tc1->y();
                }
            }
        }
    }

private:
    uint32_t posOffset;
    uint32_t texCoordOffset;
    bool wrap;
};


bool approxEqual(float x, float y, float prec)
{
    return abs(x - y) <= prec * min(abs(x), abs(y));
}


class PointEquivalencePredicate : public VertexComparator
{
public:
    PointEquivalencePredicate(uint32_t _posOffset,
                              float _tolerance) :
        posOffset(_posOffset),
        tolerance(_tolerance)
    {
    }

    bool compare(const Vertex& a, const Vertex& b) const override
    {
        const auto* adata = reinterpret_cast<const char*>(a.attributes);
        const auto* bdata = reinterpret_cast<const char*>(b.attributes);
        const Vector3f* p0 = reinterpret_cast<const Vector3f*>(adata + posOffset);
        const Vector3f* p1 = reinterpret_cast<const Vector3f*>(bdata + posOffset);

        return (approxEqual(p0->x(),  p1->x(), tolerance) &&
                approxEqual(p0->y(),  p1->y(), tolerance) &&
                approxEqual(p0->z(),  p1->z(), tolerance));
    }

private:
    uint32_t posOffset;
    float tolerance;
};


class PointTexCoordEquivalencePredicate : public VertexComparator
{
public:
    PointTexCoordEquivalencePredicate(uint32_t _posOffset,
                                      uint32_t _texCoordOffset,
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
        const auto* adata = reinterpret_cast<const char*>(a.attributes);
        const auto* bdata = reinterpret_cast<const char*>(b.attributes);
        const Vector3f* p0 = reinterpret_cast<const Vector3f*>(adata + posOffset);
        const Vector3f* p1 = reinterpret_cast<const Vector3f*>(bdata + posOffset);
        const Vector2f* tc0 = reinterpret_cast<const Vector2f*>(adata + texCoordOffset);
        const Vector2f* tc1 = reinterpret_cast<const Vector2f*>(bdata + texCoordOffset);

        return (approxEqual(p0->x(),  p1->x(), tolerance) &&
                approxEqual(p0->y(),  p1->y(), tolerance) &&
                approxEqual(p0->z(),  p1->z(), tolerance) &&
                approxEqual(tc0->x(), tc1->x(), tolerance) &&
                approxEqual(tc0->y(), tc1->y(), tolerance));
    }

private:
    uint32_t posOffset;
    uint32_t texCoordOffset;
    bool wrap;
    float tolerance;
};


bool equal(const Vertex& a, const Vertex& b, uint32_t vertexSize)
{
    const auto* s0 = reinterpret_cast<const char*>(a.attributes);
    const auto* s1 = reinterpret_cast<const char*>(b.attributes);

    for (uint32_t i = 0; i < vertexSize; i++)
    {
        if (s0[i] != s1[i])
            return false;
    }

    return true;
}


bool equalPoint(const Vertex& a, const Vertex& b)
{
    const Vector3f* p0 = reinterpret_cast<const Vector3f*>(a.attributes);
    const Vector3f* p1 = reinterpret_cast<const Vector3f*>(b.attributes);

    return *p0 == *p1;
}


bool operator==(const Mesh::VertexAttribute& a,
                const Mesh::VertexAttribute& b)
{
    return (a.semantic == b.semantic &&
            a.format   == b.format &&
            a.offset   == b.offset);
}


bool operator<(const Mesh::VertexAttribute& a,
               const Mesh::VertexAttribute& b)
{
    if (a.semantic < b.semantic)
    {
        return true;
    }
    if (b.semantic < a.semantic)
    {
        return false;
    }
    else
    {
        if (a.format < b.format)
            return true;
        if (b.format < a.format)
            return false;
        else
            return a.offset < b.offset;
    }
}


bool operator==(const Mesh::VertexDescription& a,
                const Mesh::VertexDescription& b)
{
    if (a.stride != b.stride || a.nAttributes != b.nAttributes)
        return false;

    for (uint32_t i = 0; i < a.nAttributes; i++)
    {
        if (!(a.attributes[i] == b.attributes[i]))
            return false;
    }

    return true;
}


bool operator<(const Mesh::VertexDescription& a,
               const Mesh::VertexDescription& b)
{
    if (a.stride < b.stride)
        return true;
    if (b.stride < a.stride)
        return false;

    if (a.nAttributes < b.nAttributes)
        return true;
    if (b.nAttributes < a.nAttributes)
        return false;

    for (uint32_t i = 0; i < a.nAttributes; i++)
    {
        if (a.attributes[i] < b.attributes[i])
            return true;
        else if (b.attributes[i] < a.attributes[i])
            return false;
    }

    return false;
}


struct MeshVertexDescComparator
{
    bool operator()(const Mesh* a, const Mesh* b) const
    {
        return a->getVertexDescription() < b->getVertexDescription();
    }
};


bool
UniquifyVertices(Mesh& mesh)
{
    uint32_t nVertices = mesh.getVertexCount();
    const Mesh::VertexDescription& desc = mesh.getVertexDescription();

    if (nVertices == 0)
        return false;

    const char* vertexData = reinterpret_cast<const char*>(mesh.getVertexData());
    if (vertexData == nullptr)
        return false;

    // Initialize the array of vertices
    vector<Vertex> vertices(nVertices);
    uint32_t i;
    for (i = 0; i < nVertices; i++)
    {
        vertices[i] = Vertex(i, vertexData + i * desc.stride);
    }

    // Sort the vertices so that identical ones will be ordered consecutively
    sort(vertices.begin(), vertices.end(), FullComparator(desc.stride));

    // Count the number of unique vertices
    uint32_t uniqueVertexCount = 0;
    for (i = 0; i < nVertices; i++)
    {
        if (i == 0 || !equal(vertices[i - 1], vertices[i], desc.stride))
            uniqueVertexCount++;
    }

    // No work left to do if we couldn't eliminate any vertices
    if (uniqueVertexCount == nVertices)
        return true;

    // Build the vertex map and the uniquified vertex data
    vector<uint32_t> vertexMap(nVertices);
    auto* newVertexData = new char[uniqueVertexCount * desc.stride];
    const char* oldVertexData = reinterpret_cast<const char*>(mesh.getVertexData());
    uint32_t j = 0;
    for (i = 0; i < nVertices; i++)
    {
        if (i == 0 || !equal(vertices[i - 1], vertices[i], desc.stride))
        {
            if (i != 0)
                j++;
            assert(j < uniqueVertexCount);
            memcpy(newVertexData + j * desc.stride,
                   oldVertexData + vertices[i].index * desc.stride,
                   desc.stride);
        }
        vertexMap[vertices[i].index] = j;
    }

    // Replace the vertex data with the compacted data
    mesh.setVertices(uniqueVertexCount, newVertexData);

    mesh.remapIndices(vertexMap);

    return true;
}


Vector3f
getVertex(const void* vertexData,
          int positionOffset,
          uint32_t stride,
          uint32_t index)
{
    const float* fdata = reinterpret_cast<const float*>(reinterpret_cast<const char*>(vertexData) + stride * index + positionOffset);

    return Vector3f(fdata[0], fdata[1], fdata[2]);
}


Vector2f
getTexCoord(const void* vertexData,
            int texCoordOffset,
            uint32_t stride,
            uint32_t index)
{
    const float* fdata = reinterpret_cast<const float*>(reinterpret_cast<const char*>(vertexData) + stride * index + texCoordOffset);

    return Vector2f(fdata[0], fdata[1]);
}


Vector3f
averageFaceVectors(const vector<Face>& faces,
                   uint32_t thisFace,
                   uint32_t* vertexFaces,
                   uint32_t vertexFaceCount,
                   float cosSmoothingAngle)
{
    const Face& face = faces[thisFace];

    Vector3f v = Vector3f::Zero();
    for (uint32_t i = 0; i < vertexFaceCount; i++)
    {
        uint32_t f = vertexFaces[i];
        float cosAngle = face.normal.dot(faces[f].normal);
        if (f == thisFace || cosAngle > cosSmoothingAngle)
            v += faces[f].normal;
    }

    if (v.squaredNorm() == 0.0f)
        v = Vector3f::UnitX();
    else
        v.normalize();

    return v;
}


void
copyVertex(void* newVertexData,
           const Mesh::VertexDescription& newDesc,
           const void* oldVertexData,
           const Mesh::VertexDescription& oldDesc,
           uint32_t oldIndex,
           const uint32_t fromOffsets[])
{
    const char* oldVertex = reinterpret_cast<const char*>(oldVertexData) +
        oldDesc.stride * oldIndex;
    auto* newVertex = reinterpret_cast<char*>(newVertexData);

    for (uint32_t i = 0; i < newDesc.nAttributes; i++)
    {
        if (fromOffsets[i] != ~0u)
        {
            memcpy(newVertex + newDesc.attributes[i].offset,
                   oldVertex + fromOffsets[i],
                   Mesh::getVertexAttributeSize(newDesc.attributes[i].format));
        }
    }
}


void
augmentVertexDescription(Mesh::VertexDescription& desc,
                         Mesh::VertexAttributeSemantic semantic,
                         Mesh::VertexAttributeFormat format)
{
    Mesh::VertexAttribute* attributes = new Mesh::VertexAttribute[desc.nAttributes + 1];
    uint32_t stride = 0;
    uint32_t nAttributes = 0;
    bool foundMatch = false;

    for (uint32_t i = 0; i < desc.nAttributes; i++)
    {
        if (semantic == desc.attributes[i].semantic &&
            format != desc.attributes[i].format)
        {
            // The semantic matches, but the format does not; skip this
            // item.
        }
        else
        {
            if (semantic == desc.attributes[i].semantic)
                foundMatch = true;

            attributes[nAttributes] = desc.attributes[i];
            attributes[nAttributes].offset = stride;
            stride += Mesh::getVertexAttributeSize(desc.attributes[i].format);
            nAttributes++;
        }
    }

    if (!foundMatch)
    {
        attributes[nAttributes++] = Mesh::VertexAttribute(semantic,
                                                          format,
                                                          stride);
        stride += Mesh::getVertexAttributeSize(format);
    }

    delete[] desc.attributes;
    desc.attributes = attributes;
    desc.nAttributes = nAttributes;
    desc.stride = stride;
}


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
cmod::Mesh*
GenerateNormals(const cmod::Mesh& mesh, float smoothAngle, bool weld, float weldTolerance)
{
    uint32_t nVertices = mesh.getVertexCount();
    float cosSmoothAngle = (float) cos(smoothAngle);

    const Mesh::VertexDescription& desc = mesh.getVertexDescription();

    if (desc.getAttribute(Mesh::Position).format != Mesh::Float3)
    {
        cerr << "Vertex position must be a float3\n";
        return nullptr;
    }
    uint32_t posOffset = desc.getAttribute(Mesh::Position).offset;

    uint32_t nFaces = 0;
    uint32_t i;
    for (i = 0; mesh.getGroup(i) != nullptr; i++)
    {
        const Mesh::PrimitiveGroup* group = mesh.getGroup(i);

        switch (group->prim)
        {
        case Mesh::TriList:
            if (group->nIndices < 3 || group->nIndices % 3 != 0)
            {
                cerr << "Triangle list has invalid number of indices\n";
                return nullptr;
            }
            nFaces += group->nIndices / 3;
            break;

        case Mesh::TriStrip:
        case Mesh::TriFan:
            if (group->nIndices < 3)
            {
                cerr << "Error: tri strip or fan has less than three indices\n";
                return nullptr;
            }
            nFaces += group->nIndices - 2;
            break;

        default:
            cerr << "Cannot generate normals for non-triangle primitives\n";
            return nullptr;
        }
    }

    // Build the array of faces; this may require decomposing triangle strips
    // and fans into triangle lists.
    vector<Face> faces(nFaces);

    uint32_t f = 0;
    for (i = 0; mesh.getGroup(i) != nullptr; i++)
    {
        const Mesh::PrimitiveGroup* group = mesh.getGroup(i);

        switch (group->prim)
        {
        case Mesh::TriList:
            {
                for (uint32_t j = 0; j < group->nIndices / 3; j++)
                {
                    assert(f < nFaces);
                    faces[f].i[0] = group->indices[j * 3];
                    faces[f].i[1] = group->indices[j * 3 + 1];
                    faces[f].i[2] = group->indices[j * 3 + 2];
                    f++;
                }
            }
            break;

        case Mesh::TriStrip:
            {
                for (uint32_t j = 2; j < group->nIndices; j++)
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

        case Mesh::TriFan:
            {
                for (uint32_t j = 2; j < group->nIndices; j++)
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

    const void* vertexData = mesh.getVertexData();

    // Compute normals for the faces
    for (f = 0; f < nFaces; f++)
    {
        Face& face = faces[f];
        Vector3f p0 = getVertex(vertexData, posOffset, desc.stride, face.i[0]);
        Vector3f p1 = getVertex(vertexData, posOffset, desc.stride, face.i[1]);
        Vector3f p2 = getVertex(vertexData, posOffset, desc.stride, face.i[2]);
        face.normal = (p1 - p0).cross(p2 - p1);
        if (face.normal.squaredNorm() > 0.0f)
        {
            face.normal.normalize();
        }
    }

    // For each vertex, create a list of faces that contain it
    uint32_t* faceCounts = new uint32_t[nVertices];
    uint32_t** vertexFaces = new uint32_t*[nVertices];

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
        JoinVertices(faces, vertexData, desc,
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
            vertexFaces[i] = new uint32_t[faceCounts[i] + 1];
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
    vector<Vector3f> vertexNormals(nFaces * 3);
    for (f = 0; f < nFaces; f++)
    {
        Face& face = faces[f];
        for (uint32_t j = 0; j < 3; j++)
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
    Mesh::VertexDescription newDesc(desc);
    augmentVertexDescription(newDesc, Mesh::Normal, Mesh::Float3);

    // We need to convert the copy the old vertex attributes to the new
    // mesh.  In order to do this, we need the old offset of each attribute
    // in the new vertex description.  The fromOffsets array will contain
    // this mapping.
    uint32_t normalOffset = 0;
    uint32_t fromOffsets[16];
    for (i = 0; i < newDesc.nAttributes; i++)
    {
        fromOffsets[i] = ~0;

        if (newDesc.attributes[i].semantic == Mesh::Normal)
        {
            normalOffset = newDesc.attributes[i].offset;
        }
        else
        {
            for (uint32_t j = 0; j < desc.nAttributes; j++)
            {
                if (desc.attributes[j].semantic == newDesc.attributes[i].semantic)
                {
                    assert(desc.attributes[j].format == newDesc.attributes[i].format);
                    fromOffsets[i] = desc.attributes[j].offset;
                    break;
                }
            }
        }
    }

    // Copy the old vertex data along with the generated normals to the
    // new vertex data buffer.
    void* newVertexData = new char[newDesc.stride * nFaces * 3];
    for (f = 0; f < nFaces; f++)
    {
        Face& face = faces[f];

        for (uint32_t j = 0; j < 3; j++)
        {
            char* newVertex = reinterpret_cast<char*>(newVertexData) +
                (f * 3 + j) * newDesc.stride;
            copyVertex(newVertex, newDesc,
                       vertexData, desc,
                       face.i[j],
                       fromOffsets);
            memcpy(newVertex + normalOffset, &vertexNormals[f * 3 + j],
                   Mesh::getVertexAttributeSize(Mesh::Float3));
        }
    }

    // Create the Celestia mesh
    Mesh* newMesh = new Mesh();
    newMesh->setVertexDescription(newDesc);
    newMesh->setVertices(nFaces * 3, newVertexData);

    // Create a trivial index list
    uint32_t* indices = new uint32_t[nFaces * 3];
    for (i = 0; i < nFaces * 3; i++)
    {
        indices[i] = i;
    }

    uint32_t firstIndex = 0;
    for (uint32_t groupIndex = 0; mesh.getGroup(groupIndex) != 0; ++groupIndex)
    {
        const Mesh::PrimitiveGroup* group = mesh.getGroup(groupIndex);
        unsigned int faceCount = 0;

        switch (group->prim)
        {
        case Mesh::TriList:
            faceCount = group->nIndices / 3;
            break;
        case Mesh::TriStrip:
        case Mesh::TriFan:
            faceCount = group->nIndices - 2;
            break;
        default:
            assert(0);
            break;
        }

        newMesh->addGroup(Mesh::TriList, mesh.getGroup(groupIndex)->materialIndex, faceCount * 3, indices + firstIndex);
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


Mesh*
GenerateTangents(const Mesh& mesh, bool weld)
{
    uint32_t nVertices = mesh.getVertexCount();

    // In order to generate tangents, we require positions, normals, and
    // 2D texture coordinates in the vertex description.
    const Mesh::VertexDescription& desc = mesh.getVertexDescription();
    if (desc.getAttribute(Mesh::Position).format != Mesh::Float3)
    {
        cerr << "Vertex position must be a float3\n";
        return nullptr;
    }

    if (desc.getAttribute(Mesh::Normal).format != Mesh::Float3)
    {
        cerr << "float3 format vertex normal required\n";
        return nullptr;
    }

    if (desc.getAttribute(Mesh::Texture0).format == Mesh::InvalidFormat)
    {
        cerr << "Texture coordinates must be present in mesh to generate tangents\n";
        return nullptr;
    }

    if (desc.getAttribute(Mesh::Texture0).format != Mesh::Float2)
    {
        cerr << "Texture coordinate must be a float2\n";
        return nullptr;
    }

    // Count the number of faces in the mesh.
    // (All geometry should already converted to triangle lists)
    uint32_t i;
    uint32_t nFaces = 0;
    for (i = 0; mesh.getGroup(i) != nullptr; i++)
    {
        const Mesh::PrimitiveGroup* group = mesh.getGroup(i);
        if (group->prim == Mesh::TriList)
        {
            assert(group->nIndices % 3 == 0);
            nFaces += group->nIndices / 3;
        }
        else
        {
            cerr << "Mesh should contain just triangle lists\n";
            return nullptr;
        }
    }

    // Build the array of faces; this may require decomposing triangle strips
    // and fans into triangle lists.
    vector<Face> faces(nFaces);

    uint32_t f = 0;
    for (i = 0; mesh.getGroup(i) != nullptr; i++)
    {
        const Mesh::PrimitiveGroup* group = mesh.getGroup(i);

        switch (group->prim)
        {
        case Mesh::TriList:
            {
                for (uint32_t j = 0; j < group->nIndices / 3; j++)
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

    uint32_t posOffset = desc.getAttribute(Mesh::Position).offset;
    uint32_t texCoordOffset = desc.getAttribute(Mesh::Texture0).offset;

    const void* vertexData = mesh.getVertexData();

    // Compute tangents for faces
    for (f = 0; f < nFaces; f++)
    {
        Face& face = faces[f];
        Vector3f p0 = getVertex(vertexData, posOffset, desc.stride, face.i[0]);
        Vector3f p1 = getVertex(vertexData, posOffset, desc.stride, face.i[1]);
        Vector3f p2 = getVertex(vertexData, posOffset, desc.stride, face.i[2]);
        Vector2f tc0 = getTexCoord(vertexData, texCoordOffset, desc.stride, face.i[0]);
        Vector2f tc1 = getTexCoord(vertexData, texCoordOffset, desc.stride, face.i[1]);
        Vector2f tc2 = getTexCoord(vertexData, texCoordOffset, desc.stride, face.i[2]);
        float s1 = tc1.x() - tc0.x();
        float s2 = tc2.x() - tc0.x();
        float t1 = tc1.y() - tc0.y();
        float t2 = tc2.y() - tc0.y();
        float a = s1 * t2 - s2 * t1;
        if (a != 0.0f)
            face.normal = (t2 * (p1 - p0) - t1 * (p2 - p0)) * (1.0f / a);
        else
            face.normal = Vector3f::Zero();
    }

    // For each vertex, create a list of faces that contain it
    uint32_t* faceCounts = new uint32_t[nVertices];
    uint32_t** vertexFaces = new uint32_t*[nVertices];

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
        JoinVertices(faces, vertexData, desc,
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
            vertexFaces[i] = new uint32_t[faceCounts[i] + 1];
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
    vector<Vector3f> vertexTangents(nFaces * 3);
    for (f = 0; f < nFaces; f++)
    {
        Face& face = faces[f];
        for (uint32_t j = 0; j < 3; j++)
        {
            vertexTangents[f * 3 + j] =
                averageFaceVectors(faces, f,
                                   &vertexFaces[face.vi[j]][1],
                                   vertexFaces[face.vi[j]][0],
                                   0.0f);
        }
    }

    // Create the new vertex description
    Mesh::VertexDescription newDesc(desc);
    augmentVertexDescription(newDesc, Mesh::Tangent, Mesh::Float3);

    // We need to convert the copy the old vertex attributes to the new
    // mesh.  In order to do this, we need the old offset of each attribute
    // in the new vertex description.  The fromOffsets array will contain
    // this mapping.
    uint32_t tangentOffset = 0;
    uint32_t fromOffsets[16];
    for (i = 0; i < newDesc.nAttributes; i++)
    {
        fromOffsets[i] = ~0;

        if (newDesc.attributes[i].semantic == Mesh::Tangent)
        {
            tangentOffset = newDesc.attributes[i].offset;
        }
        else
        {
            for (uint32_t j = 0; j < desc.nAttributes; j++)
            {
                if (desc.attributes[j].semantic == newDesc.attributes[i].semantic)
                {
                    assert(desc.attributes[j].format == newDesc.attributes[i].format);
                    fromOffsets[i] = desc.attributes[j].offset;
                    break;
                }
            }
        }
    }

    // Copy the old vertex data along with the generated tangents to the
    // new vertex data buffer.
    void* newVertexData = new char[newDesc.stride * nFaces * 3];
    for (f = 0; f < nFaces; f++)
    {
        Face& face = faces[f];

        for (uint32_t j = 0; j < 3; j++)
        {
            char* newVertex = reinterpret_cast<char*>(newVertexData) +
                (f * 3 + j) * newDesc.stride;
            copyVertex(newVertex, newDesc,
                       vertexData, desc,
                       face.i[j],
                       fromOffsets);
            memcpy(newVertex + tangentOffset, &vertexTangents[f * 3 + j],
                   Mesh::getVertexAttributeSize(Mesh::Float3));
        }
    }

    // Create the Celestia mesh
    Mesh* newMesh = new Mesh();
    newMesh->setVertexDescription(newDesc);
    newMesh->setVertices(nFaces * 3, newVertexData);

    // Create a trivial index list
    uint32_t* indices = new uint32_t[nFaces * 3];
    for (i = 0; i < nFaces * 3; i++)
        indices[i] = i;

    uint32_t firstIndex = 0;
    for (uint32_t groupIndex = 0; mesh.getGroup(groupIndex) != 0; ++groupIndex)
    {
        const Mesh::PrimitiveGroup* group = mesh.getGroup(groupIndex);
        unsigned int faceCount = 0;

        switch (group->prim)
        {
        case Mesh::TriList:
            faceCount = group->nIndices / 3;
            break;
        case Mesh::TriStrip:
        case Mesh::TriFan:
            faceCount = group->nIndices - 2;
            break;
        default:
            assert(0);
            break;
        }

        newMesh->addGroup(Mesh::TriList, mesh.getGroup(groupIndex)->materialIndex, faceCount * 3, indices + firstIndex);
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


void
addGroupWithOffset(Mesh& mesh,
                   const Mesh::PrimitiveGroup& group,
                   uint32_t offset)
{
    if (group.nIndices == 0)
        return;

    uint32_t* newIndices = new uint32_t[group.nIndices];
    for (uint32_t i = 0; i < group.nIndices; i++)
        newIndices[i] = group.indices[i] + offset;

    mesh.addGroup(group.prim, group.materialIndex,
                  group.nIndices, newIndices);
}


// Merge all meshes that share the same vertex description
Model*
MergeModelMeshes(const Model& model)
{
    vector<Mesh*> meshes;

    for (uint32_t i = 0; model.getMesh(i) != nullptr; i++)
    {
        meshes.push_back(model.getMesh(i));
    }

    // Sort the meshes by vertex description
    sort(meshes.begin(), meshes.end(), MeshVertexDescComparator());

    Model* newModel = new Model();

    // Copy materials into the new model
    for (uint32_t i = 0; model.getMaterial(i) != nullptr; i++)
    {
        newModel->addMaterial(model.getMaterial(i));
    }

    uint32_t meshIndex = 0;
    while (meshIndex < meshes.size())
    {
        const Mesh::VertexDescription& desc =
            meshes[meshIndex]->getVertexDescription();

        // Count the number of matching meshes
        uint32_t nMatchingMeshes;
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
        uint32_t totalVertices = 0;
        uint32_t j;
        for (j = meshIndex; j < meshIndex + nMatchingMeshes; j++)
        {
            totalVertices += meshes[j]->getVertexCount();
        }

        char* vertexData = new char[totalVertices * desc.stride];

        // Create the new empty mesh
        Mesh* mergedMesh = new Mesh();
        mergedMesh->setVertexDescription(desc);
        mergedMesh->setVertices(totalVertices, vertexData);

        // Copy the vertex data and reindex and add primitive groups
        uint32_t vertexCount = 0;
        for (j = meshIndex; j < meshIndex + nMatchingMeshes; j++)
        {
            const Mesh* mesh = meshes[j];
            memcpy(vertexData + vertexCount * desc.stride,
                   mesh->getVertexData(),
                   mesh->getVertexCount() * desc.stride);

            for (uint32_t k = 0; mesh->getGroup(k) != nullptr; k++)
            {
                addGroupWithOffset(*mergedMesh, *mesh->getGroup(k),
                                   vertexCount);
            }

            vertexCount += mesh->getVertexCount();
        }
        assert(vertexCount == totalVertices);

        newModel->addMesh(mergedMesh);

        meshIndex += nMatchingMeshes;
    }

    return newModel;
}


static Material*
cloneMaterial(const Material* other)
{
    Material* material = new Material();
    material->diffuse  = other->diffuse;
    material->specular = other->specular;
    material->emissive = other->emissive;
    material->specularPower = other->specularPower;
    material->opacity  = other->opacity;
    material->blend    = other->blend;
    for (int i = 0; i < Material::TextureSemanticMax; ++i)
    {
        if (other->maps[i])
        {
            material->maps[i] = new Material::DefaultTextureResource(other->maps[i]->source());
        }
    }

    return material;
}


/*! Generate normals for an entire model. Return the new model, or null if
 *  normal generation failed due to an out of memory error.
 */
Model*
GenerateModelNormals(const Model& model, float smoothAngle, bool weldVertices, float weldTolerance)
{
    Model* newModel = new Model();

    // Copy materials
    for (unsigned int i = 0; model.getMaterial(i) != nullptr; i++)
    {
        newModel->addMaterial(cloneMaterial(model.getMaterial(i)));
    }

    bool ok = true;
    for (unsigned int i = 0; model.getMesh(i) != nullptr; i++)
    {
        Mesh* mesh = model.getMesh(i);
        Mesh* newMesh = nullptr;

        newMesh = GenerateNormals(*mesh, smoothAngle, weldVertices, weldTolerance);
        if (newMesh == nullptr)
        {
            ok = false;
        }
        else
        {
            newModel->addMesh(newMesh);
        }
    }

    if (!ok)
    {
        // If all of the meshes weren't processed due to an out of memory error,
        // delete the new model and return nullptr rather than a partially processed
        // model.
        delete newModel;
        newModel = nullptr;
    }

    return newModel;
}


#ifdef TRISTRIP
bool
convertToStrips(Mesh& mesh)
{
    vector<Mesh::PrimitiveGroup*> groups;

    // NvTriStrip library can only handle 16-bit indices
    if (mesh.getVertexCount() >= 0x10000)
    {
        return true;
    }

    // Verify that the mesh contains just tri strips
    uint32_t i;
    for (i = 0; mesh.getGroup(i) != nullptr; i++)
    {
        if (mesh.getGroup(i)->prim != Mesh::TriList)
            return true;
    }

    // Convert the existing groups to triangle strips
    for (i = 0; mesh.getGroup(i) != nullptr; i++)
    {
        const Mesh::PrimitiveGroup* group = mesh.getGroup(i);

        // Convert the vertex indices to shorts for the TriStrip library
        unsigned short* indices = new unsigned short[group->nIndices];
        uint32_t j;
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
            cerr << "Generate tri strips failed\n";
            return false;
        }

        // Call the tristrip library to convert the lists to strips.  Then,
        // convert from the NvTriStrip's primitive group structure to the
        // CMOD one and add it to the collection that will be added once
        // the mesh's original primitive groups are cleared.
        for (j = 0; j < nGroups; j++)
        {
            Mesh::PrimitiveGroupType prim = Mesh::InvalidPrimitiveGroupType;
            switch (strips[j].type)
            {
            case PT_LIST:
                prim = Mesh::TriList;
                break;
            case PT_STRIP:
                prim = Mesh::TriStrip;
                break;
            case PT_FAN:
                prim = Mesh::TriFan;
                break;
            }

            if (prim != Mesh::InvalidPrimitiveGroupType &&
                strips[j].numIndices != 0)
            {
                Mesh::PrimitiveGroup* newGroup = new Mesh::PrimitiveGroup();
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
    for (vector<Mesh::PrimitiveGroup*>::const_iterator iter = groups.begin();
         iter != groups.end(); iter++)
    {
        mesh.addGroup(*iter);
    }

    return true;
}
#endif

#if 0
    if (genNormals || genTangents)
    {
        Model* newModel = new Model();
        uint32_t i;

        // Copy materials
        for (i = 0; model->getMaterial(i) != nullptr; i++)
        {
            newModel->addMaterial(model->getMaterial(i));
        }

        // Generate normals and/or tangents for each model in the mesh
        for (i = 0; model->getMesh(i) != nullptr; i++)
        {
            Mesh* mesh = model->getMesh(i);
            Mesh* newMesh = nullptr;

            if (genNormals)
            {
                newMesh = generateNormals(*mesh,
                                          degToRad(smoothAngle),
                                          weldVertices);
                if (newMesh == nullptr)
                {
                    cerr << "Error generating normals!\n";
                    return 1;
                }
                // TODO: clean up old mesh
                mesh = newMesh;
            }

            if (genTangents)
            {
                newMesh = generateTangents(*mesh, weldVertices);
                if (newMesh == nullptr)
                {
                    cerr << "Error generating tangents!\n";
                    return 1;
                }
                // TODO: clean up old mesh
                mesh = newMesh;
            }

            newModel->addMesh(mesh);
        }

        // delete model;
        model = newModel;
    }

    if (mergeMeshes)
    {
        model = mergeModelMeshes(*model);
    }

    if (uniquify)
    {
        for (uint32_t i = 0; model->getMesh(i) != nullptr; i++)
        {
            Mesh* mesh = model->getMesh(i);
            uniquifyVertices(*mesh);
        }
    }

#ifdef TRISTRIP
    if (stripify)
    {
        SetCacheSize(vertexCacheSize);
        for (uint32_t i = 0; model->getMesh(i) != nullptr; i++)
        {
            Mesh* mesh = model->getMesh(i);
            convertToStrips(*mesh);
        }
    }
#endif

#endif
