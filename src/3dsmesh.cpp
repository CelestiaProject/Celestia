// 3dsmesh.cpp
// 
// Copyright (C) 2001, Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include "gl.h"
#include "3dsmesh.h"

using namespace std;


static TriangleList* convertTriangleMesh(M3DTriangleMesh& mesh,
                                         const M3DScene& scene);

Mesh3DS::Mesh3DS(const M3DScene& scene)
{
    for (int i = 0; i < scene.getModelCount(); i++)
    {
        M3DModel* model = scene.getModel(i);
        if (model != NULL)
        {
            for (int j = 0; j < model->getTriMeshCount(); j++)
            {
                M3DTriangleMesh* mesh = model->getTriMesh(j);
                if (mesh != NULL)
                {
                    triLists.insert(triLists.end(),
                                    convertTriangleMesh(*mesh, scene));
                }
            }
        }
    }
}


Mesh3DS::~Mesh3DS()
{
    for (TriListVec::iterator i = triLists.begin(); i != triLists.end(); i++)
        if (*i != NULL)
            delete *i;
}


void Mesh3DS::render()
{
    for (TriListVec::iterator i = triLists.begin(); i != triLists.end(); i++)
        (*i)->render();
}


void Mesh3DS::normalize()
{
    AxisAlignedBox bbox;

    for (TriListVec::iterator i = triLists.begin(); i != triLists.end(); i++)
        bbox.include((*i)->getBoundingBox());

    Point3f center = bbox.getCenter();
    Vec3f extents = bbox.getExtents();
    float maxExtent = extents.x;
    if (extents.y > maxExtent)
        maxExtent = extents.y;
    if (extents.z > maxExtent)
        maxExtent = extents.z;

    printf("Normalize: %f\n", maxExtent);

    for (i = triLists.begin(); i != triLists.end(); i++)
        (*i)->transform(Point3f(0, 0, 0) - center, 1.0f / maxExtent);
}


static TriangleList* convertTriangleMesh(M3DTriangleMesh& mesh,
                                         const M3DScene& scene)
{
    TriangleList* tl = new TriangleList();
    int nFaces = mesh.getFaceCount();
    int nVertices = mesh.getVertexCount();
    bool smooth = (mesh.getSmoothingGroupCount() == nFaces);
    int i;
    
    Vec3f* faceNormals = new Vec3f[nFaces];
    Vec3f* vertexNormals = new Vec3f[nFaces * 3];
    int* faceCounts = new int[nVertices];
    int** vertexFaces = new int*[nVertices];

    for (i = 0; i < nVertices; i++)
    {
        faceCounts[i] = 0;
        vertexFaces[i] = NULL;
    }

    // generate face normals
    for (i = 0; i < nFaces; i++)
    {
        uint16 v0, v1, v2;
        mesh.getFace(i, v0, v1, v2);

        faceCounts[v0]++;
        faceCounts[v1]++;
        faceCounts[v2]++;

        Point3f p0 = mesh.getVertex(v0);
        Point3f p1 = mesh.getVertex(v1);
        Point3f p2 = mesh.getVertex(v2);
        faceNormals[i] = cross(p1 - p0, p2 - p1);
        faceNormals[i].normalize();
    }

    if (!smooth && 0)
    {
        for (i = 0; i < nFaces; i++)
        {
            vertexNormals[i * 3] = faceNormals[i];
            vertexNormals[i * 3 + 1] = faceNormals[i];
            vertexNormals[i * 3 + 2] = faceNormals[i];
        }
    }
    else
    {
        // allocate space for vertex face indices
        for (i = 0; i < nVertices; i++)
        {
            vertexFaces[i] = new int[faceCounts[i] + 1];
            vertexFaces[i][0] = faceCounts[i];
        }

        for (i = 0; i < nFaces; i++)
        {
            uint16 v0, v1, v2;
            mesh.getFace(i, v0, v1, v2);
            vertexFaces[v0][faceCounts[v0]--] = i;
            vertexFaces[v1][faceCounts[v1]--] = i;
            vertexFaces[v2][faceCounts[v2]--] = i;
        }

#if 0
        for (i = 0; i < nVertices; i++)
        {
            printf("%d: ", i);
            for (int j = 1; j < vertexFaces[i][0]; j++)
            {
                printf("%d ", vertexFaces[i][j]);
            }
            printf("\n");
        }
#endif

        // average face normals to compute the vertex normals
        for (i = 0; i < nFaces; i++)
        {
            uint16 v0, v1, v2;
            mesh.getFace(i, v0, v1, v2);
            uint32 smoothingGroups = mesh.getSmoothingGroups(i);

            int j;
            Vec3f v = Vec3f(0, 0, 0);
            for (j = 1; j <= vertexFaces[v0][0]; j++)
            {
                int k = vertexFaces[v0][j];
                // if (k == i || (smoothingGroups & mesh.getSmoothingGroups(k)) != 0)
                if (faceNormals[i] * faceNormals[k] > 0.5f)
                    v += faceNormals[k];
            }
            v.normalize();
            vertexNormals[i * 3] = v;

            v = Vec3f(0, 0, 0);
            for (j = 1; j <= vertexFaces[v1][0]; j++)
            {
                int k = vertexFaces[v1][j];
                // if (k == i || (smoothingGroups & mesh.getSmoothingGroups(k)) != 0)
                if (faceNormals[i] * faceNormals[k] > 0.5f)
                    v += faceNormals[k];
            }
            v.normalize();
            vertexNormals[i * 3 + 1] = v;

            v = Vec3f(0, 0, 0);
            for (j = 1; j <= vertexFaces[v2][0]; j++)
            {
                int k = vertexFaces[v2][j];
                // if (k == i || (smoothingGroups & mesh.getSmoothingGroups(k)) != 0)
                if (faceNormals[i] * faceNormals[k] > 0.5f)
                    v += faceNormals[k];
            }
            v.normalize();
            vertexNormals[i * 3 + 2] = v;
        }
    }

    // build the triangle list
    for (i = 0; i < nFaces; i++)
    {
        uint16 v0, v1, v2;
        mesh.getFace(i, v0, v1, v2);

        tl->addTriangle(mesh.getVertex(v0), vertexNormals[i * 3],
                        mesh.getVertex(v1), vertexNormals[i * 3 + 1],
                        mesh.getVertex(v2), vertexNormals[i * 3 + 2]);
    }

    // set the color
    {
        string materialName = mesh.getMaterialName();
        if (materialName.length() > 0)
        {
            tl->setColorMode(1);
            int nMaterials = scene.getMaterialCount();
            for (i = 0; i < nMaterials; i++)
            {
                M3DMaterial* material = scene.getMaterial(i);
                if (materialName == material->getName())
                {
                    M3DColor diffuse = material->getDiffuseColor();
                    tl->setColor(Vec3f(diffuse.red, diffuse.green, diffuse.blue));
                    break;
                }
            }
        }
    }

    // clean up
    if (faceNormals != NULL)
        delete[] faceNormals;
    if (vertexNormals != NULL)
        delete[] vertexNormals;
    if (faceCounts != NULL)
        delete[] faceCounts;
    if (vertexFaces != NULL)
    {
        for (i = 0; i < nVertices; i++)
        {
            if (vertexFaces[i] != NULL)
                delete[] vertexFaces[i];
        }
        delete[] vertexFaces;
    }

    return tl;
}
