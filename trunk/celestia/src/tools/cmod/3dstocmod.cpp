// 3dstocmod.cpp
//
// Copyright (C) 2004, Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// Convert a 3DS file to a Celestia mesh (.cmod) file

#include <celengine/modelfile.h>
#include <celengine/tokenizer.h>
#include <celengine/texmanager.h>
#include <cel3ds/3dsread.h>
#include <cstring>
#include <cassert>
#include <cmath>
#include <cstdio>


static Model* Convert3DSModel(const M3DScene& scene, const string& texPath);


void usage()
{
    cerr << "Usage: 3dstocmod <input 3ds file>\n";
}


int main(int argc, char* argv[])
{
    if (argc != 2)
    {
        usage();
        return 1;
    }

    string inputFileName = argv[1];

    M3DScene* scene = Read3DSFile(inputFileName);
    if (scene == NULL)
    {
        cerr << "Error reading 3DS file '" << inputFileName << "'\n";
        return 1;
    }

    Model* model = Convert3DSModel(*scene, ".");
    if (!model)
    {
        cerr << "Error converting 3DS file to Celestia model\n";
        return 1;
    }

    for (uint32 i = 0; model->getMesh(i); i++)
    {
        const Mesh* mesh = model->getMesh(i);
        for (uint32 j = 0; mesh->getGroup(j); j++)
        {
            const Mesh::PrimitiveGroup* group = mesh->getGroup(j);
            cerr << "Group: #" << i << ", indices=" << group->nIndices << '\n';
        }
    }

    SaveModelAscii(model, cout);

    return 0;
}


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
    bool hasTexCoords = (nTexCoords >= nVertices);
    int vertexSize = hasTexCoords ? 8 : 6;
    int i;

    Vec3f* faceNormals = new Vec3f[nFaces];
    Vec3f* vertexNormals = new Vec3f[nFaces * 3];
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

        Point3f p0 = mesh3ds.getVertex(v0);
        Point3f p1 = mesh3ds.getVertex(v1);
        Point3f p2 = mesh3ds.getVertex(v2);
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
            Vec3f v = Vec3f(0, 0, 0);
            for (j = 1; j <= vertexFaces[v0][0]; j++)
            {
                int k = vertexFaces[v0][j];
                // if (k == i || (smoothingGroups & mesh3ds.getSmoothingGroups(k)) != 0)
                if (faceNormals[i] * faceNormals[k] > 0.5f)
                    v += faceNormals[k];
            }
            if (v * v == 0.0f)
                v = Vec3f(1.0f, 0.0f, 0.0f);
            v.normalize();
            vertexNormals[i * 3] = v;

            v = Vec3f(0, 0, 0);
            for (j = 1; j <= vertexFaces[v1][0]; j++)
            {
                int k = vertexFaces[v1][j];
                // if (k == i || (smoothingGroups & mesh3ds.getSmoothingGroups(k)) != 0)
                if (faceNormals[i] * faceNormals[k] > 0.5f)
                    v += faceNormals[k];
            }
            if (v * v == 0.0f)
                v = Vec3f(1.0f, 0.0f, 0.0f);
            v.normalize();
            vertexNormals[i * 3 + 1] = v;

            v = Vec3f(0, 0, 0);
            for (j = 1; j <= vertexFaces[v2][0]; j++)
            {
                int k = vertexFaces[v2][j];
                // if (k == i || (smoothingGroups & mesh3ds.getSmoothingGroups(k)) != 0)
                if (faceNormals[i] * faceNormals[k] > 0.5f)
                    v += faceNormals[k];
            }
            if (v * v == 0.0f)
                v = Vec3f(1.0f, 0.0f, 0.0f);
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
            Point3f pos = mesh3ds.getVertex(triVert[j]);
            Vec3f norm = vertexNormals[i * 3 + j];
            int k = (i * 3 + j) * vertexSize;

            vertices[k + 0] = pos.x;
            vertices[k + 1] = pos.y;
            vertices[k + 2] = pos.z;
            vertices[k + 3] = norm.x;
            vertices[k + 4] = norm.y;
            vertices[k + 5] = norm.z;
            if (hasTexCoords)
            {
                vertices[k + 6] = mesh3ds.getTexCoord(triVert[j]).x;
                vertices[k + 7] = mesh3ds.getTexCoord(triVert[j]).y;
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

    // Vertex lists are not indexed, so the conversion to an indexed format is
    // trivial (although much space is wasted storing unnecessary indices.)
    uint32* indices = new uint32[nOutputVertices];
    for (int i = 0; i < nOutputVertices; i++)
        indices[i] = i;

    // Convert the 3DS mesh's material
    Mesh::Material* material = new Mesh::Material();

    string material3dsName = mesh3ds.getMaterialName();
    if (material3dsName.length() > 0)
    {
        int nMaterials = scene.getMaterialCount();
        for (int i = 0; i < nMaterials; i++)
        {
            M3DMaterial* material3ds = scene.getMaterial(i);
            if (material3dsName == material3ds->getName())
            {
                M3DColor diffuse = material3ds->getDiffuseColor();
                material->diffuse = Color(diffuse.red, diffuse.green, diffuse.blue);
                M3DColor specular = material3ds->getSpecularColor();
                material->specular = Color(specular.red, specular.green, specular.blue);
                // Map the shininess from the 3DS file into the 0-128
                // range that OpenGL uses for the specular exponent.
                float specPow = (float) pow(2.0, 1.0 + 0.1 * material3ds->getShininess());
                if (specPow > 128.0f)
                    specPow = 128.0f;
                material->specularPower = specPow;

                material->opacity = material3ds->getOpacity();
                if (material3ds->getTextureMap() != "")
                {
                    material->maps[Mesh::DiffuseMap] = GetTextureManager()->getHandle(TextureInfo(material3ds->getTextureMap(), ".", TextureInfo::WrapTexture));
                }
            }
        }
    }

    uint32 materialIndex = model.addMaterial(material) - 1;
    mesh->addGroup(Mesh::TriList, materialIndex, nOutputVertices, indices);
    model.addMesh(mesh);
}


static Model*
Convert3DSModel(const M3DScene& scene, const string& texPath)
{
    Model* model = new Model();
    uint32 materialIndex = 0;

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

