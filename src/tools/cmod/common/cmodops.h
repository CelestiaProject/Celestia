// cmodops.h
//
// Copyright (C) 2004-2010, Chris Laurel <claurel@gmail.com>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// Perform various adjustments to a Celestia mesh.

#pragma once

#include <algorithm>
#include <cassert>
#include <cstdint>
#include <vector>

#include <Eigen/Core>

#include <celmodel/mesh.h>

namespace cmod
{
class Model;
}


struct Vertex
{
    Vertex() :
        index(0), attributes(nullptr) {};

    Vertex(std::uint32_t _index, const void* _attributes) :
        index(_index), attributes(_attributes) {};

    std::uint32_t index;
    const void* attributes;
};


struct Face
{
    Eigen::Vector3f normal;
    std::uint32_t i[3];    // vertex attribute indices
    std::uint32_t vi[3];   // vertex point indices -- same as above unless welding
};


// Mesh operations
extern cmod::Mesh* GenerateNormals(const cmod::Mesh& mesh, float smoothAngle, bool weld, float weldTolerance = 0.0f);
extern cmod::Mesh* GenerateTangents(const cmod::Mesh& mesh, bool weld);
extern bool UniquifyVertices(cmod::Mesh& mesh);

// Model operations
extern cmod::Model* MergeModelMeshes(const cmod::Model& model);
extern cmod::Model* GenerateModelNormals(const cmod::Model& model, float smoothAngle, bool weldVertices, float weldTolerance);
#ifdef TRISTRIP
extern bool ConvertToStrips(cmod::Mesh& mesh);
#endif

template<typename T, typename U> void
JoinVertices(std::vector<Face>& faces,
             const void* vertexData,
             const cmod::VertexDescription& desc,
             const T& orderingPredicate,
             const U& equivalencePredicate)
{
    // Don't do anything if we're given no data
    if (faces.size() == 0)
        return;

    // Must have a position
    assert(desc.getAttribute(cmod::Mesh::Position).format == cmod::Mesh::Float3);

    std::uint32_t posOffset = desc.getAttribute(cmod::VertexAttributeSemantic::Position).offset;
    const char* vertexPoints = reinterpret_cast<const char*>(vertexData) + posOffset;
    std::uint32_t nVertices = faces.size() * 3;

    // Initialize the array of vertices
    std::vector<Vertex> vertices(nVertices);
    std::uint32_t f;
    for (f = 0; f < faces.size(); f++)
    {
        for (std::uint32_t j = 0; j < 3; j++)
        {
            std::uint32_t index = faces[f].i[j];
            vertices[f * 3 + j] = Vertex(index,
                                         vertexPoints + desc.stride * index);

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
