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


static Material*
convert3dsMaterial(const M3DMaterial* material3ds)
{
    Material* newMaterial = new Material();

    M3DColor diffuse = material3ds->getDiffuseColor();
    newMaterial->diffuse = Material::Color(diffuse.red, diffuse.green, diffuse.blue);
    newMaterial->opacity = material3ds->getOpacity();

    M3DColor specular = material3ds->getSpecularColor();
    newMaterial->specular = Material::Color(specular.red, specular.green, specular.blue);

    float shininess = material3ds->getShininess();

    // Map the 3DS file's shininess from percentage (0-100) to
    // range that OpenGL uses for the specular exponent. The
    // current equation is just a guess at the mapping that
    // 3DS actually uses.
    newMaterial->specularPower = (float) pow(2.0, 1.0 + 0.1 * shininess);
    if (newMaterial->specularPower > 128.0f)
        newMaterial->specularPower = 128.0f;

    if (!material3ds->getTextureMap().empty())
    {
        newMaterial->maps[Material::DiffuseMap] = new Material::DefaultTextureResource(material3ds->getTextureMap());

    }

    return newMaterial;
}


void
Convert3DSMesh(Model& model,
               M3DTriangleMesh& mesh3ds,
               const M3DScene& scene,
               const string& meshName)
{
    int nVertices = mesh3ds.getVertexCount();
    int nTexCoords = mesh3ds.getTexCoordCount();
    bool hasTexCoords = (nTexCoords >= nVertices);
    int vertexSize = 3;
    if (hasTexCoords)
    {
        vertexSize += 2;
    }

    float* vertices = new float[mesh3ds.getVertexCount() * vertexSize];

    // Build the vertex list
    for (int i = 0; i < mesh3ds.getVertexCount(); ++i)
    {
        int k = i * vertexSize;
        Vector3f pos = mesh3ds.getVertex(i);
        vertices[k + 0] = pos.x();
        vertices[k + 1] = pos.y();
        vertices[k + 2] = pos.z();

        if (hasTexCoords)
        {
            Vector2f texCoord = mesh3ds.getTexCoord(i);
            vertices[k + 3] = texCoord.x();
            vertices[k + 4] = texCoord.y();
        }
    }

    Mesh::VertexAttribute attributes[8];
    uint32 nAttributes = 0;
    uint32 offset = 0;

    // Position attribute is always present
    attributes[nAttributes] = Mesh::VertexAttribute(Mesh::Position, Mesh::Float3, 0);
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
    mesh->setVertices(nVertices, vertices);

    mesh->setName(meshName);

    if (mesh3ds.getMeshMaterialGroupCount() == 0)
    {
        // No material groups in the 3DS file. This is allowed. We'll create a single
        // primitive group with the default material.
        unsigned int faceCount = mesh3ds.getFaceCount();
        uint32* indices = new uint32[faceCount * 3];

        for (unsigned int i = 0; i < faceCount; i++)
        {
            uint16 v0 = 0, v1 = 0, v2 = 0;
            mesh3ds.getFace(i, v0, v1, v2);
            indices[i * 3 + 0] = v0;
            indices[i * 3 + 1] = v1;
            indices[i * 3 + 2] = v2;
        }

        mesh->addGroup(Mesh::TriList, ~0, faceCount * 3, indices);
    }
    else
    {
        // We have at least one material group. Create a cmod primitive group for
        // each material group in th 3ds mesh.
        for (uint32 groupIndex = 0; groupIndex < mesh3ds.getMeshMaterialGroupCount(); ++groupIndex)
        {
            M3DMeshMaterialGroup* matGroup = mesh3ds.getMeshMaterialGroup(groupIndex);

            uint32 nMatGroupFaces = matGroup->faces.size();
            uint32* indices = new uint32[nMatGroupFaces * 3];

            for (unsigned int i = 0; i < nMatGroupFaces; i++)
            {
                uint16 v0 = 0, v1 = 0, v2 = 0;
                uint16 faceIndex = matGroup->faces[i];
                mesh3ds.getFace(faceIndex, v0, v1, v2);
                indices[i * 3 + 0] = v0;
                indices[i * 3 + 1] = v1;
                indices[i * 3 + 2] = v2;
            }

            // Get the material index
            unsigned int materialIndex = ~0u;
            string material3dsName = matGroup->materialName;
            if (!material3dsName.empty())
            {
                for (unsigned int i = 0; i < scene.getMaterialCount(); i++)
                {
                    M3DMaterial* material3ds = scene.getMaterial(i);
                    if (material3dsName == material3ds->getName())
                    {
                        materialIndex = i;
                    }
                }
            }

            mesh->addGroup(Mesh::TriList, materialIndex, nMatGroupFaces * 3, indices);
        }
    }

    model.addMesh(mesh);
}


Model*
Convert3DSModel(const M3DScene& scene)
{
    Model* model = new Model();

    // Convert materials
    for (unsigned int i = 0; i < scene.getMaterialCount(); i++)
    {
        M3DMaterial* material = scene.getMaterial(i);
        model->addMaterial(convert3dsMaterial(material));
    }

    // Convert meshes
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
}

