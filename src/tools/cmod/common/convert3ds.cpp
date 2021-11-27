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

#include <cmath>
#include <cstdint>

#include <Eigen/Core>

#include <celmodel/material.h>
#include <celmodel/mesh.h>

#include "convert3ds.h"

namespace
{
cmod::Material*
convert3dsMaterial(const M3DMaterial* material3ds, cmod::HandleGetter& handleGetter)
{
    cmod::Material* newMaterial = new cmod::Material();

    M3DColor diffuse = material3ds->getDiffuseColor();
    newMaterial->diffuse = cmod::Color(diffuse.red, diffuse.green, diffuse.blue);
    newMaterial->opacity = material3ds->getOpacity();

    M3DColor specular = material3ds->getSpecularColor();
    newMaterial->specular = cmod::Color(specular.red, specular.green, specular.blue);

    float shininess = material3ds->getShininess();

    // Map the 3DS file's shininess from percentage (0-100) to
    // range that OpenGL uses for the specular exponent. The
    // current equation is just a guess at the mapping that
    // 3DS actually uses.
    newMaterial->specularPower = std::pow(2.0f, 1.0f + 0.1f * shininess);
    if (newMaterial->specularPower > 128.0f)
        newMaterial->specularPower = 128.0f;

    if (!material3ds->getTextureMap().empty())
    {
        newMaterial->setMap(cmod::TextureSemantic::DiffuseMap, handleGetter(material3ds->getTextureMap()));

    }

    return newMaterial;
}
} // end unnamed namespace





void
Convert3DSMesh(cmod::Model& model,
               const M3DTriangleMesh& mesh3ds,
               const M3DScene& scene,
               const std::string& meshName)
{
    int nVertices = mesh3ds.getVertexCount();
    int nTexCoords = mesh3ds.getTexCoordCount();
    bool hasTexCoords = (nTexCoords >= nVertices);
    int vertexSize = 3;
    if (hasTexCoords)
    {
        vertexSize += 2;
    }

    auto* vertices = new float[mesh3ds.getVertexCount() * vertexSize];

    // Build the vertex list
    for (int i = 0; i < mesh3ds.getVertexCount(); ++i)
    {
        int k = i * vertexSize;
        Eigen::Vector3f pos = mesh3ds.getVertex(i);
        vertices[k + 0] = pos.x();
        vertices[k + 1] = pos.y();
        vertices[k + 2] = pos.z();

        if (hasTexCoords)
        {
            Eigen::Vector2f texCoord = mesh3ds.getTexCoord(i);
            vertices[k + 3] = texCoord.x();
            vertices[k + 4] = texCoord.y();
        }
    }

    cmod::VertexAttribute attributes[8];
    std::uint32_t nAttributes = 0;
    std::uint32_t offset = 0;

    // Position attribute is always present
    attributes[nAttributes] = cmod::VertexAttribute(cmod::VertexAttributeSemantic::Position,
                                                    cmod::VertexAttributeFormat::Float3,
                                                    0);
    nAttributes++;
    offset += 12;

    if (hasTexCoords)
    {
        attributes[nAttributes] = cmod::VertexAttribute(cmod::VertexAttributeSemantic::Texture0,
                                                        cmod::VertexAttributeFormat::Float2,
                                                        offset);
        nAttributes++;
        offset += 8;
    }

    // Create the Celestia mesh
    cmod::Mesh* mesh = new cmod::Mesh();
    mesh->setVertexDescription(cmod::VertexDescription(offset, nAttributes, attributes));
    mesh->setVertices(nVertices, vertices);

    mesh->setName(meshName);

    if (mesh3ds.getMeshMaterialGroupCount() == 0)
    {
        // No material groups in the 3DS file. This is allowed. We'll create a single
        // primitive group with the default material.
        unsigned int faceCount = mesh3ds.getFaceCount();
        auto* indices = new std::uint32_t[faceCount * 3];

        for (unsigned int i = 0; i < faceCount; i++)
        {
            std::uint16_t v0 = 0, v1 = 0, v2 = 0;
            mesh3ds.getFace(i, v0, v1, v2);
            indices[i * 3 + 0] = v0;
            indices[i * 3 + 1] = v1;
            indices[i * 3 + 2] = v2;
        }

        mesh->addGroup(cmod::PrimitiveGroupType::TriList, ~0, faceCount * 3, indices);
    }
    else
    {
        // We have at least one material group. Create a cmod primitive group for
        // each material group in th 3ds mesh.
        for (std::uint32_t groupIndex = 0; groupIndex < mesh3ds.getMeshMaterialGroupCount(); ++groupIndex)
        {
            const M3DMeshMaterialGroup* matGroup = mesh3ds.getMeshMaterialGroup(groupIndex);

            std::uint32_t nMatGroupFaces = matGroup->faces.size();
            auto* indices = new std::uint32_t[nMatGroupFaces * 3];

            for (unsigned int i = 0; i < nMatGroupFaces; i++)
            {
                std::uint16_t v0 = 0, v1 = 0, v2 = 0;
                std::uint16_t faceIndex = matGroup->faces[i];
                mesh3ds.getFace(faceIndex, v0, v1, v2);
                indices[i * 3 + 0] = v0;
                indices[i * 3 + 1] = v1;
                indices[i * 3 + 2] = v2;
            }

            // Get the material index
            unsigned int materialIndex = ~0u;
            std::string material3dsName = matGroup->materialName;
            if (!material3dsName.empty())
            {
                for (unsigned int i = 0; i < scene.getMaterialCount(); i++)
                {
                    const M3DMaterial* material3ds = scene.getMaterial(i);
                    if (material3dsName == material3ds->getName())
                    {
                        materialIndex = i;
                    }
                }
            }

            mesh->addGroup(cmod::PrimitiveGroupType::TriList, materialIndex, nMatGroupFaces * 3, indices);
        }
    }

    model.addMesh(mesh);
}


cmod::Model*
Convert3DSModel(const M3DScene& scene, cmod::HandleGetter handleGetter)
{
    cmod::Model* model = new cmod::Model();

    // Convert materials
    for (unsigned int i = 0; i < scene.getMaterialCount(); i++)
    {
        const M3DMaterial* material = scene.getMaterial(i);
        model->addMaterial(convert3dsMaterial(material, handleGetter));
    }

    // Convert meshes
    for (unsigned int i = 0; i < scene.getModelCount(); i++)
    {
        const M3DModel* model3ds = scene.getModel(i);
        if (model3ds != nullptr)
        {
            for (unsigned int j = 0; j < model3ds->getTriMeshCount(); j++)
            {
                const M3DTriangleMesh* mesh = model3ds->getTriMesh(j);

                if (mesh != nullptr && mesh->getFaceCount() > 0)
                {
                    Convert3DSMesh(*model, *mesh, scene, model3ds->getName());
                }
            }
        }
    }

    return model;
}

