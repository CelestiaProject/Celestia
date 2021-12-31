// spheremesh.cpp
//
// Copyright (C) 2001-2009, the Celestia Development Team
// Original version by Chris Laurel <claurel@gmail.com>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <utility>

#include <celcompat/numbers.h>
#include <celmath/mathlib.h>
#include <celmath/randutils.h>
#include <celmodel/mesh.h>

#include "spheremesh.h"


namespace {

constexpr const int MinRings = 3;
constexpr const int MinSlices = 3;

} // end unnamed namespace

float SphereMeshParameters::value(float u, float v) const
{
    float theta = u * celestia::numbers::pi_v<float> * 2;
    float phi = (v - 0.5f) * celestia::numbers::pi_v<float>;

    Eigen::Vector3f p = Eigen::Vector3f(std::cos(phi) * std::cos(theta),
                                        std::sin(phi),
                                        std::cos(phi) * std::sin(theta))
                      + offset;
    return celmath::fractalsum(p, octaves) * featureHeight;
}

SphereMesh::SphereMesh(const Eigen::Vector3f& size,
                       int _nRings, int _nSlices,
                       const SphereMeshParameters& params) :
    nRings(std::max(_nRings, MinRings)),
    nSlices(std::max(_nSlices, MinSlices)),
    nVertices(nRings * (nSlices + 1))
{
    createSphere();
    scale(size);
    displace(params);
    generateNormals();
    fixNormals();
}

void SphereMesh::createSphere()
{
    vertices.reserve(nVertices);
    normals.reserve(nVertices);
    texCoords.reserve(nVertices);

    auto nRings1f = static_cast<float>(nRings - 1);
    auto nSlicesf = static_cast<float>(nSlices);

    for (int i = 0; i < nRings; i++)
    {
        float phi = (static_cast<float>(i) / static_cast<float>(nRings - 1) - 0.5f) * celestia::numbers::pi_v<float>;
        for (int j = 0; j <= nSlices; j++)
        {
            float theta = static_cast<float>(j) / static_cast<float>(nSlices) * static_cast<float>(celestia::numbers::pi * 2.0);
            auto x = std::cos(phi) * std::cos(theta);
            auto y = std::sin(phi);
            auto z = std::cos(phi) * std::sin(theta);
            vertices.emplace_back(x, y, z);
            normals.emplace_back(x, y, z);
            texCoords.emplace_back(1.0f - static_cast<float>(j) / nSlicesf,
                                   1.0f - static_cast<float>(i) / nRings1f);
        }
    }
}

void SphereMesh::scale(const Eigen::Vector3f& s)
{
    for (Eigen::Vector3f& vertex : vertices)
    {
        vertex = vertex.cwiseProduct(s);
    }

    // TODO: Make a fast special case for uniform scale factors, where
    // renormalization is not required.
    Eigen::Vector3f is = s.cwiseInverse();
    for (Eigen::Vector3f& normal : normals)
    {
        normal = normal.cwiseProduct(is).normalized();
    }
}

void SphereMesh::displace(const SphereMeshParameters& params)
{
    auto nRings1f = static_cast<float>(nRings - 1);
    auto nSlicesf = static_cast<float>(nSlices);
    for (int i = 0; i < nRings; i++)
    {
        float v = static_cast<float>(i) / nRings1f;
        for (int j = 0; j <= nSlices; j++)
        {
            float u = static_cast<float>(j) / nSlicesf;
            int n = i * (nSlices + 1) + j;
            vertices[n] += normals[n] * params.value(u, v);
        }
    }
}

// Generate vertex normals for a quad mesh by averaging face normals
void SphereMesh::generateNormals()
{
    int nQuads = nSlices * (nRings - 1);
    std::vector<Eigen::Vector3f> faceNormals;
    faceNormals.reserve(nQuads);

    // Compute face normals for the mesh
    for (int i = 0; i < nRings - 1; i++)
    {
        for (int j = 0; j < nSlices; j++)
        {
            const Eigen::Vector3f& p0 = vertices[i * (nSlices + 1) + j];
            const Eigen::Vector3f& p1 = vertices[(i + 1) * (nSlices + 1) + j];
            const Eigen::Vector3f& p2 = vertices[(i + 1) * (nSlices + 1) + j + 1];
            const Eigen::Vector3f& p3 = vertices[i * (nSlices + 1) + j + 1];

            // Compute the face normal.  Watch out for degenerate (zero-length)
            // edges.  If there are two degenerate edges, the entire face must
            // be degenerate and we'll handle that later
            Eigen::Vector3f v0 = p1 - p0;
            Eigen::Vector3f v1 = p2 - p1;
            if (v0.norm() < 1e-6f)
            {
                v0 = p2 - p1;
                v1 = p3 - p2;
            }
            else if (v1.norm() < 1e-6f)
            {
                v0 = p3 - p2;
                v1 = p0 - p3;
            }

            Eigen::Vector3f faceNormal = v0.cross(v1);
            float length = faceNormal.norm();
            if (length != 0)
                faceNormal *= (1 / length);

            faceNormals.push_back(faceNormal);
        }
    }

    std::vector<int> faceCounts(nVertices, 4);
    for (int i = 1; i < nRings - 1; i++)
    {
        for (int j = 0; j <= nSlices; j++)
        {
            int n = i * (nSlices + 1) + j;

            normals[n] = faceNormals[(i - 1) * nSlices + j % nSlices]
                       + faceNormals[(i - 1) * nSlices + (j + nSlices - 1) % nSlices]
                       + faceNormals[i * nSlices + (j + nSlices - 1) % nSlices]
                       + faceNormals[i * nSlices + j % nSlices];
        }
    }

    // Compute normals at the poles
    for (int i = 0; i <= nSlices; i++)
    {
        int vertex = i;
        faceCounts[vertex] = nSlices;
        normals[vertex] = Eigen::Vector3f::Zero();
        for (int j = 0; j < nSlices; j++)
        {
            int face = j;
            normals[vertex] += faceNormals[face];
        }

        vertex = nVertices - nSlices - 1 + i;
        faceCounts[vertex] = nSlices;
        normals[vertex] = Eigen::Vector3f::Zero();
        for (int j = 0; j < nSlices; j++)
        {
            int face = nQuads - j - 1;
            normals[vertex] += faceNormals[face];
        }
    }

    for (Eigen::Vector3f& normal : normals)
    {
        normal.normalize();
    }
}

// Fix up the normals along the seam at longitude zero
void SphereMesh::fixNormals()
{
    for (int i = 0; i < nRings; i++)
    {
        int idx0 = i * (nSlices + 1);
        int idx1 = (i + 1) * (nSlices + 1) - 1;
        Eigen::Vector3f normal = (normals[idx0] + normals[idx1]).normalized();
        normals[idx0] = normal;
        normals[idx1] = normal;
    }
}

cmod::Mesh SphereMesh::convertToMesh() const
{
    static_assert(sizeof(float) == sizeof(cmod::VWord), "Float size mismatch with vertex data word size");
    constexpr std::uint32_t stride = 8;

    std::vector<cmod::VertexAttribute> attributes{
        cmod::VertexAttribute(cmod::VertexAttributeSemantic::Position,
                              cmod::VertexAttributeFormat::Float3,
                              0),
        cmod::VertexAttribute(cmod::VertexAttributeSemantic::Normal,
                              cmod::VertexAttributeFormat::Float3,
                              3),
        cmod::VertexAttribute(cmod::VertexAttributeSemantic::Texture0,
                              cmod::VertexAttributeFormat::Float2,
                              6),
    };

    cmod::Mesh mesh;

    mesh.setVertexDescription(cmod::VertexDescription(std::move(attributes)));

    // Copy the vertex data from the separate position, normal, and texture coordinate
    // arrays into a single array.
    std::vector<cmod::VWord> vertexData(stride * nVertices);

    for (int i = 0; i < nVertices; i++)
    {
        cmod::VWord* vertex = vertexData.data() + stride * i;
        std::memcpy(vertex, vertices[i].data(), sizeof(Eigen::Vector3f));
        std::memcpy(vertex + 3, normals[i].data(), sizeof(Eigen::Vector3f));
        std::memcpy(vertex + 6, texCoords[i].data(), sizeof(Eigen::Vector2f));
    }

    mesh.setVertices(nVertices, std::move(vertexData));

    for (int i = 0; i < nRings - 1; i++)
    {
        std::vector<cmod::Index32> indexData;
        indexData.reserve((nSlices + 1) * 2);
        for (int j = 0; j <= nSlices; j++)
        {
            indexData.push_back(i * (nSlices + 1) + j);
            indexData.push_back((i + 1) * (nSlices + 1) + j);
        }

        mesh.addGroup(cmod::PrimitiveGroupType::TriStrip, ~0u, std::move(indexData));
    }

    return mesh;
}
