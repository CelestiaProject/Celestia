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

#ifndef _CMODOPS_H_
#define _CMODOPS_H_

#include <celmodel/model.h>
#include <celutil/basictypes.h>
#include <Eigen/Core>
#include <vector>


struct Vertex
{
    Vertex() :
        index(0), attributes(NULL) {};

    Vertex(uint32 _index, const void* _attributes) :
        index(_index), attributes(_attributes) {};

    uint32 index;
    const void* attributes;
};


struct Face
{
    Eigen::Vector3f normal;
    uint32 i[3];    // vertex attribute indices
    uint32 vi[3];   // vertex point indices -- same as above unless welding
};


// Mesh operations
extern cmod::Mesh* GenerateNormals(const cmod::Mesh& mesh, float smoothAngle, bool weld, float weldTolerance = 0.0f);
extern cmod::Mesh* GenerateTangents(const cmod::Mesh& mesh, bool weld);
extern bool UniquifyVertices(cmod::Mesh& mesh);

// Model operations
extern cmod::Model* MergeModelMeshes(const cmod::Model& model);
extern cmod::Model* GenerateModelNormals(const cmod::Model& model, float smoothAngle, bool weldVertices, float weldTolerance);


template<typename T, typename U> void
JoinVertices(std::vector<Face>& faces,
             const void* vertexData,
             const cmod::Mesh::VertexDescription& desc,
             const T& orderingPredicate,
             const U& equivalencePredicate)
{
    // Don't do anything if we're given no data
    if (faces.size() == 0)
        return;

    // Must have a position
    assert(desc.getAttribute(cmod::Mesh::Position).format == cmod::Mesh::Float3);

    uint32 posOffset = desc.getAttribute(cmod::Mesh::Position).offset;
    const char* vertexPoints = reinterpret_cast<const char*>(vertexData) +
        posOffset;
    uint32 nVertices = faces.size() * 3;

    // Initialize the array of vertices
    std::vector<Vertex> vertices(nVertices);
    uint32 f;
    for (f = 0; f < faces.size(); f++)
    {
        for (uint32 j = 0; j < 3; j++)
        {
            uint32 index = faces[f].i[j];
            vertices[f * 3 + j] = Vertex(index,
                                         vertexPoints + desc.stride * index);
                                         
        }
    }

    // Sort the vertices so that identical ones will be ordered consecutively
    sort(vertices.begin(), vertices.end(), orderingPredicate);

    // Build the vertex merge map
    std::vector<uint32> mergeMap(nVertices);
    uint32 lastUnique = 0;
    uint32 uniqueCount = 0;
    for (uint32 i = 0; i < nVertices; i++)
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
        for (uint32 k= 0; k < 3; k++)
        {
            faces[f].vi[k] = mergeMap[faces[f].i[k]];
        }
    }
}

#endif // _CMODOPS_H_
