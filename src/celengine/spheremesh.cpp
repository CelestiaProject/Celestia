// mesh.cpp
//
// Copyright (C) 2000, Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include <cmath>
#include <celmath/mathlib.h>
#include <celmath/vecmath.h>
#include "gl.h"
#include "glext.h"
#include "spheremesh.h"


SphereMesh::SphereMesh(float radius, int _nRings, int _nSlices) :
    vertices(NULL), normals(NULL), texCoords(NULL), indices(NULL)
{
    createSphere(radius, _nRings, _nSlices);
}

SphereMesh::SphereMesh(Vec3f size, int _nRings, int _nSlices) :
    vertices(NULL), normals(NULL), texCoords(NULL), indices(NULL)
{
    createSphere(1.0f, _nRings, _nSlices);
    scale(size);
}

SphereMesh::SphereMesh(Vec3f size,
                       const DisplacementMap& dispmap,
                       float height) :
    vertices(NULL), normals(NULL), texCoords(NULL), indices(NULL)
{
    createSphere(1.0f, dispmap.getHeight(), dispmap.getWidth());
    scale(size);
    displace(dispmap, height);
    generateNormals();
    fixNormals();
}

SphereMesh::SphereMesh(Vec3f size,
                       int _nRings, int _nSlices,
                       DisplacementMapFunc func,
                       void* info)
{
    createSphere(1.0f, _nRings, _nSlices);
    scale(size);
    displace(func, info);
    generateNormals();
    fixNormals();
}

SphereMesh::~SphereMesh()
{
    if (vertices != NULL)
        delete[] vertices;
    if (normals != NULL)
        delete[] normals;
    if (texCoords != NULL)
        delete[] texCoords;
    if (indices != NULL)
        delete[] indices;
}


void SphereMesh::createSphere(float radius, int _nRings, int _nSlices)
{
    nRings = _nRings;
    nSlices = _nSlices;
    nVertices = nRings * (nSlices + 1);
    vertices = new float[nVertices * 3];
    normals = new float[nVertices * 3];
    texCoords = new float[nVertices * 2];
    nIndices = (nRings - 1) * (nSlices + 1) * 2;
    indices = new unsigned short[nIndices];
    tangents = new float[nVertices * 3];

    int i;
    for (i = 0; i < nRings; i++)
    {
        float phi = ((float) i / (float) (nRings - 1) - 0.5f) * (float) PI;
        for (int j = 0; j <= nSlices; j++)
        {
            float theta = (float) j / (float) nSlices * (float) PI * 2;
            int n = i * (nSlices + 1) + j;
            float x = (float) (cos(phi) * cos(theta));
            float y = (float) sin(phi);
            float z = (float) (cos(phi) * sin(theta));
            vertices[n * 3]      = x * radius;
            vertices[n * 3 + 1]  = y * radius;
            vertices[n * 3 + 2]  = z * radius;
            normals[n * 3]       = x;
            normals[n * 3 + 1]   = y;
            normals[n * 3 + 2]   = z;
            texCoords[n * 2]     = 1.0f - (float) j / (float) nSlices;
            texCoords[n * 2 + 1] = 1.0f - (float) i / (float) (nRings - 1);

            // Compute the tangent--required for bump mapping
            float tx = (float) (sin(phi) * sin(theta));
            float ty = (float) -cos(phi);
            float tz = (float) (sin(phi) * cos(theta));
            tangents[n * 3]      = tx;
            tangents[n * 3 + 1]  = ty;
            tangents[n * 3 + 2]  = tz;
        }
    }

    for (i = 0; i < nRings - 1; i++)
    {
        for (int j = 0; j <= nSlices; j++)
        {
            int n = i * (nSlices + 1) + j;
            indices[n * 2 + 0] = i * (nSlices + 1) + j;
            indices[n * 2 + 1] = (i + 1) * (nSlices + 1) + j;
        }
    }
}


// Generate vertex normals for a quad mesh by averaging face normals
void SphereMesh::generateNormals()
{
    int nQuads = nSlices * (nRings - 1);
    Vec3f* faceNormals = new Vec3f[nQuads];
    int i;

    // Compute face normals for the mesh
    for (i = 0; i < nRings - 1; i++)
    {
        for (int j = 0; j < nSlices; j++)
        {
            float* p0 = vertices + (i * (nSlices + 1) + j) * 3;
            float* p1 = vertices + ((i + 1) * (nSlices + 1) + j) * 3;
            float* p2 = vertices + ((i + 1) * (nSlices + 1) + j + 1) * 3;
            float* p3 = vertices + (i * (nSlices + 1) + j + 1) * 3;

            // Compute the face normal.  Watch out for degenerate (zero-length)
            // edges.  If there are two degenerate edges, the entire face must
            // be degenerate and we'll handle that later
            Vec3f v0(p1[0] - p0[0], p1[1] - p0[1], p1[2] - p0[2]);
            Vec3f v1(p2[0] - p1[0], p2[1] - p1[1], p2[2] - p1[2]);
            if (v0.length() < 1e-6f)
            {
                v0 = Vec3f(p2[0] - p1[0], p2[1] - p1[1], p2[2] - p1[2]);
                v1 = Vec3f(p3[0] - p2[0], p3[1] - p2[1], p3[2] - p2[2]);
            }
            else if (v1.length() < 1e-6f)
            {
                v0 = Vec3f(p3[0] - p2[0], p3[1] - p2[1], p3[2] - p2[2]);
                v1 = Vec3f(p0[0] - p3[0], p0[1] - p3[1], p0[2] - p3[2]);
            }

            Vec3f faceNormal = cross(v0, v1);
            float length = faceNormal.length();
            if (length != 0)
                faceNormal *= (1 / length);

            faceNormals[i * nSlices + j] = faceNormal;
        }
    }

    int* faceCounts = new int[nVertices];
    for (i = 0; i < nVertices; i++)
    {
        faceCounts[i] = 0;
        normals[i * 3] = 0;
        normals[i * 3 + 1] = 0;
        normals[i * 3 + 2] = 0;
    }

    for (i = 1; i < nRings - 1; i++)
    {
        for (int j = 0; j <= nSlices; j++)
        {
            int vertex = i * (nSlices + 1) + j;
            faceCounts[vertex] = 4;

            int face = (i - 1) * nSlices + j % nSlices;
            normals[vertex * 3]     += faceNormals[face].x;
            normals[vertex * 3 + 1] += faceNormals[face].y;
            normals[vertex * 3 + 2] += faceNormals[face].z;
            face = (i - 1) * nSlices + (j + nSlices - 1) % nSlices;
            normals[vertex * 3]     += faceNormals[face].x;
            normals[vertex * 3 + 1] += faceNormals[face].y;
            normals[vertex * 3 + 2] += faceNormals[face].z;
            face = i * nSlices + (j + nSlices - 1) % nSlices;
            normals[vertex * 3]     += faceNormals[face].x;
            normals[vertex * 3 + 1] += faceNormals[face].y;
            normals[vertex * 3 + 2] += faceNormals[face].z;
            face = i * nSlices + j % nSlices;
            normals[vertex * 3]     += faceNormals[face].x;
            normals[vertex * 3 + 1] += faceNormals[face].y;
            normals[vertex * 3 + 2] += faceNormals[face].z;
        }
    }

    // Compute normals at the poles
    for (i = 0; i <= nSlices; i++)
    {
        int vertex = i;
        int j;
        faceCounts[vertex] = nSlices;
        for (j = 0; j < nSlices; j++)
        {
            int face = j;
            normals[vertex * 3]     += faceNormals[face].x;
            normals[vertex * 3 + 1] += faceNormals[face].y;
            normals[vertex * 3 + 2] += faceNormals[face].z;
        }

        vertex = (nRings - 1) * (nSlices + 1) + i;
        faceCounts[vertex] = nSlices;
        for (j = 0; j < nSlices; j++)
        {
            int face = nQuads - j - 1;
            normals[vertex * 3]     += faceNormals[face].x;
            normals[vertex * 3 + 1] += faceNormals[face].y;
            normals[vertex * 3 + 2] += faceNormals[face].z;
        }
    }

    for (i = 0; i < nVertices; i++)
    {
        if (faceCounts[i] > 0)
        {
            float s = 1.0f / (float) faceCounts[i];
            float nx = normals[i * 3] * s;
            float ny = normals[i * 3 + 1] * s;
            float nz = normals[i * 3 + 2] * s;
            float length = (float) sqrt(nx * nx + ny * ny + nz * nz);
            if (length > 0)
            {
                length = 1 / length;
                normals[i * 3]     = nx * length;
                normals[i * 3 + 1] = ny * length;
                normals[i * 3 + 2] = nz * length;
            }
        }
    }

    delete[] faceCounts;
    delete[] faceNormals;
}


// Fix up the normals along the seam at longitude zero
void SphereMesh::fixNormals()
{
    for (int i = 0; i < nRings; i++)
    {
        float* v0 = normals + (i * (nSlices + 1)) * 3;
        float* v1 = normals + ((i + 1) * (nSlices + 1) - 1) * 3;
        Vec3f n0(v0[0], v0[1], v0[2]);
        Vec3f n1(v0[0], v0[1], v0[2]);
        Vec3f normal = n0 + n1;
        normal.normalize();
        v0[0] = normal.x;
        v0[1] = normal.y;
        v0[2] = normal.z;
        v1[0] = normal.x;
        v1[1] = normal.y;
        v1[2] = normal.z;
    }
}


void SphereMesh::scale(Vec3f s)
{
    int i;
    for (i = 0; i < nVertices; i++)
    {
        vertices[i * 3]     *= s.x;
        vertices[i * 3 + 1] *= s.y;
        vertices[i * 3 + 2] *= s.z;
    }

    // Modify the normals
    if (normals != NULL)
    {
        // TODO: Make a fast special case for uniform scale factors, where
        // renormalization is not required.
        Vec3f is(1.0f / s.x, 1.0f / s.y, 1.0f / s.z);
        for (i = 0; i < nVertices; i++)
        {
            int n = i * 3;
            Vec3f normal(normals[n] * is.x, normals[n + 1] * is.y, normals[n + 2] * is.z);
            normal.normalize();
            normals[n]     = normal.x;
            normals[n + 1] = normal.y;
            normals[n + 2] = normal.z;
        }
    }
}


void SphereMesh::displace(const DisplacementMap& dispmap,
                          float height)
{
    // assert(dispMap.getWidth() == nSlices);
    // assert(dispMap.getHeight() == nRings);

    for (int i = 0; i < nRings; i++)
    {
        for (int j = 0; j <= nSlices; j++)
        {
            int n = (i * (nSlices + 1) + j) * 3;
            /*
            float theta = (float) j / (float) nSlices * (float) PI * 2;
            float x = (float) (cos(phi) * cos(theta));
            float y = (float) sin(phi);
            float z = (float) (cos(phi) * sin(theta));
            */
            Vec3f normal(normals[n], normals[n + 1], normals[n + 2]);

            int k = (j == nSlices) ? 0 : j;
            Vec3f v = normal * dispmap.getDisplacement(k, i) * height;
            vertices[n] += v.x;
            vertices[n + 1] += v.y;
            vertices[n + 2] += v.z;
        }
    }
}


void SphereMesh::displace(DisplacementMapFunc func, void* info)
{
    for (int i = 0; i < nRings; i++)
    {
        float v = (float) i / (float) (nRings - 1);
        for (int j = 0; j <= nSlices; j++)
        {
            float u = (float) j / (float) nSlices;
            int n = (i * (nSlices + 1) + j) * 3;
            Vec3f normal(normals[n], normals[n + 1], normals[n + 2]);
            Vec3f vert = normal * func(u, v, info);
            vertices[n] += vert.x;
            vertices[n + 1] += vert.y;
            vertices[n + 2] += vert.z;
        }
    }
}


Mesh* SphereMesh::convertToMesh() const
{
    uint32 stride = 32;
    Mesh::VertexAttribute attributes[3];
    attributes[0] = Mesh::VertexAttribute(Mesh::Position, Mesh::Float3, 0);
    attributes[1] = Mesh::VertexAttribute(Mesh::Normal, Mesh::Float3, 12);
    attributes[2] = Mesh::VertexAttribute(Mesh::Texture0, Mesh::Float2, 24);

    Mesh* mesh = new Mesh();

    mesh->setVertexDescription(Mesh::VertexDescription(stride, 3, attributes));

    // Copy the vertex data from the separate position, normal, and texture coordinate
    // arrays into a single array.
    char* vertexData = new char[stride * nVertices];

    int i;
    for (i = 0; i < nVertices; i++)
    {
        float* vertex = reinterpret_cast<float*>(vertexData + stride * i);
        vertex[0] = vertices[i * 3];
        vertex[1] = vertices[i * 3 + 1];
        vertex[2] = vertices[i * 3 + 2];
        vertex[3] = normals[i * 3];
        vertex[4] = normals[i * 3 + 1];
        vertex[5] = normals[i * 3 + 2];
        vertex[6] = texCoords[i * 2];
        vertex[7] = texCoords[i * 2 + 1];
    }

    mesh->setVertices(nVertices, vertexData);

    for (i = 0; i < nRings - 1; i++)
    {
        uint32* indexData = new uint32[(nSlices + 1) * 2];
        for (int j = 0; j <= nSlices; j++)
        {
            indexData[j * 2 + 0] = i * (nSlices + 1) + j;
            indexData[j * 2 + 1] = (i + 1) * (nSlices + 1) + j;
        }

        mesh->addGroup(Mesh::TriStrip, ~0u, (nSlices + 1) * 2, indexData);
    }

    return mesh;
}
