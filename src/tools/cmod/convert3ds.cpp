// convert3ds.cpp
//
// Copyright (C) 2004-2010, Chris Laurel <claurel@gmail.com>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// Functions for converting a 3DS scene into a Celestia model (cmod)

#include "convert3ds.h"
#include <Eigen/Core>

using namespace cmod;
using namespace Eigen;
using namespace std;


void
Convert3DSMesh(Model& model,
               M3DTriangleMesh& mesh3ds,
               const M3DScene& scene,
               const string& meshName)
{
    int nFaces = mesh3ds.getFaceCount();
    int nVertices = mesh3ds.getVertexCount();
    int nTexCoords = mesh3ds.getTexCoordCount();
    bool smooth = (mesh3ds.getSmoothingGroupCount() == nFaces);
    bool hasTexCoords = (nTexCoords == nVertices);
    int vertexSize = hasTexCoords ? 8 : 6;
    int i;

    Vector3f* faceNormals = new Vector3f[nFaces];
    Vector3f* vertexNormals = new Vector3f[nFaces * 3];
    int* faceCounts = new int[nVertices];
    int** vertexFaces = new int*[nVertices];

    int nOutputVertices = nFaces * 3;
    float* vertices = new float[nOutputVertices * vertexSize];
    

    for (i = 0; i < nVertices; i++)
    {
        faceCounts[i] = 0;
        vertexFaces[i] = NULL;
    }

    // generate face normals
    for (i = 0; i < nFaces; i++)
    {
        uint16 v0, v1, v2;
        mesh3ds.getFace(i, v0, v1, v2);

        faceCounts[v0]++;
        faceCounts[v1]++;
        faceCounts[v2]++;

        Vector3f p0 = mesh3ds.getVertex(v0);
        Vector3f p1 = mesh3ds.getVertex(v1);
        Vector3f p2 = mesh3ds.getVertex(v2);
        faceNormals[i] = (p1 - p0).cross(p2 - p1);
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
            mesh3ds.getFace(i, v0, v1, v2);
            vertexFaces[v0][faceCounts[v0]--] = i;
            vertexFaces[v1][faceCounts[v1]--] = i;
            vertexFaces[v2][faceCounts[v2]--] = i;
        }

        // average face normals to compute the vertex normals
        for (i = 0; i < nFaces; i++)
        {
            uint16 v0, v1, v2;
            mesh3ds.getFace(i, v0, v1, v2);
            // uint32 smoothingGroups = mesh3ds.getSmoothingGroups(i);

            int j;
            Vector3f v = Vector3f(0, 0, 0);
            for (j = 1; j <= vertexFaces[v0][0]; j++)
            {
                int k = vertexFaces[v0][j];
                // if (k == i || (smoothingGroups & mesh3ds.getSmoothingGroups(k)) != 0)
                if (faceNormals[i].dot(faceNormals[k]) > 0.5f)
                {
                    v += faceNormals[k];
                }
            }
            if (v.squaredNorm() == 0.0f)
            {
                v = Vector3f::UnitX();
            }
            v.normalize();
            vertexNormals[i * 3] = v;

            v = Vector3f(0, 0, 0);
            for (j = 1; j <= vertexFaces[v1][0]; j++)
            {
                int k = vertexFaces[v1][j];
                // if (k == i || (smoothingGroups & mesh3ds.getSmoothingGroups(k)) != 0)
                if (faceNormals[i].dot(faceNormals[k]) > 0.5f)
                {
                    v += faceNormals[k];
                }
            }
            if (v.squaredNorm() == 0.0f)
            {
                v = Vector3f::UnitX();
            }

            v.normalize();
            vertexNormals[i * 3 + 1] = v;

            v = Vector3f(0, 0, 0);
            for (j = 1; j <= vertexFaces[v2][0]; j++)
            {
                int k = vertexFaces[v2][j];
                // if (k == i || (smoothingGroups & mesh3ds.getSmoothingGroups(k)) != 0)
                if (faceNormals[i].dot(faceNormals[k]) > 0.5f)
                {
                    v += faceNormals[k];
                }
            }
            if (v.squaredNorm() == 0.0f)
            {
                v = Vector3f::UnitX();
            }
            v.normalize();
            vertexNormals[i * 3 + 2] = v;
        }
    }

    // build the triangle list
    for (i = 0; i < nFaces; i++)
    {
        uint16 triVert[3];
        mesh3ds.getFace(i, triVert[0], triVert[1], triVert[2]);

        for (int j = 0; j < 3; j++)
        {
            Vector3f pos = mesh3ds.getVertex(triVert[j]);
            Vector3f norm = vertexNormals[i * 3 + j];
            int k = (i * 3 + j) * vertexSize;

            vertices[k + 0] = pos.x();
            vertices[k + 1] = pos.y();
            vertices[k + 2] = pos.z();
            vertices[k + 3] = norm.x();
            vertices[k + 4] = norm.y();
            vertices[k + 5] = norm.z();
            if (hasTexCoords)
            {
                vertices[k + 6] = mesh3ds.getTexCoord(triVert[j]).x();
                vertices[k + 7] = mesh3ds.getTexCoord(triVert[j]).y();
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


    Mesh::VertexAttribute attributes[8];
    uint32 nAttributes = 0;
    uint32 offset = 0;

    // Position attribute is always present
    attributes[nAttributes] = Mesh::VertexAttribute(Mesh::Position, Mesh::Float3, 0);
    nAttributes++;
    offset += 12;

    // Normal attribute is always present
    attributes[nAttributes] = Mesh::VertexAttribute(Mesh::Normal, Mesh::Float3, offset);
    nAttributes++;
    offset += 12;

    if (hasTexCoords)
    {
        attributes[nAttributes] = Mesh::VertexAttribute(Mesh::Texture0, Mesh::Float2, offset);
        nAttributes++;
        offset += 8;
    }

    // Create the Celestia mesh
    Mesh* mesh = new Mesh();
    mesh->setVertexDescription(Mesh::VertexDescription(offset, nAttributes, attributes));
    mesh->setVertices(nOutputVertices, vertices);

    mesh->setName(meshName);

    for (uint32 groupIndex = 0; groupIndex < mesh3ds.getMeshMaterialGroupCount(); ++groupIndex)
    {
        M3DMeshMaterialGroup* matGroup = mesh3ds.getMeshMaterialGroup(groupIndex);
	
        // Vertex lists are not indexed, so the conversion to an indexed format is
        // trivial (although much space is wasted storing unnecessary indices.)
        uint32 nMatGroupFaces = matGroup->faces.size();
        uint32 nMatGroupVertices = nMatGroupFaces * 3;
        uint32* indices = new uint32[nMatGroupVertices];

        for (unsigned int i = 0; i < nMatGroupFaces; i++)
        {
            uint16 faceIndex = matGroup->faces[i];
            indices[i * 3 + 0] = faceIndex * 3 + 0;
            indices[i * 3 + 1] = faceIndex * 3 + 1;
            indices[i * 3 + 2] = faceIndex * 3 + 2;
        }

        // Convert the 3DS mesh's material
        Material* material = new Material();

        string material3dsName = matGroup->materialName;
        if (material3dsName.length() > 0)
        {
            int nMaterials = scene.getMaterialCount();
            for (int i = 0; i < nMaterials; i++)
            {
                M3DMaterial* material3ds = scene.getMaterial(i);

                if (material3dsName == material3ds->getName())
                {
                    M3DColor diffuse = material3ds->getDiffuseColor();
                    material->diffuse = Material::Color(diffuse.red, diffuse.green, diffuse.blue);
                    M3DColor specular = material3ds->getSpecularColor();
                    material->specular = Material::Color(specular.red, specular.green, specular.blue);
                    // Map the shininess from the 3DS file into the 0-128
                    // range that OpenGL uses for the specular exponent.
                    float specPow = (float) pow(2.0, 1.0 + 0.1 * material3ds->getShininess());
                    if (specPow > 128.0f)
                    {
                        specPow = 128.0f;
                    }
                    material->specularPower = specPow;

                    material->opacity = material3ds->getOpacity();
                    if (material3ds->getTextureMap() != "")
                    {
                        material->maps[Material::DiffuseMap] = new Material::DefaultTextureResource(material3ds->getTextureMap());
                    }
                }
            }

            uint32 materialIndex = model.addMaterial(material) - 1;
            mesh->addGroup(Mesh::TriList, materialIndex, nMatGroupVertices, indices);
        }
    }

    model.addMesh(mesh);
}


Model*
Convert3DSModel(const M3DScene& scene)
{
    Model* model = new Model();

    for (unsigned int i = 0; i < scene.getModelCount(); i++)
    {
        M3DModel* model3ds = scene.getModel(i);
        if (model3ds != NULL)
        {
            for (unsigned int j = 0; j < model3ds->getTriMeshCount(); j++)
            {
                M3DTriangleMesh* mesh = model3ds->getTriMesh(j);

                if (mesh != NULL && mesh->getFaceCount() > 0)
                {
                    Convert3DSMesh(*model, *mesh, scene, model3ds->getName());
                }
            }
        }
    }

    return model;
#if 0
    // Sort the vertex lists to make sure that the transparent ones are
    // rendered after the opaque ones and material state changes are minimized.
    sort(vertexLists.begin(), vertexLists.end(), compareVertexLists);
#endif
}

