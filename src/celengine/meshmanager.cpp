// meshmanager.cpp
//
// Copyright (C) 2001-2010, Celestia Development Team
// Original version by Chris Laurel <claurel@gmail.com>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

// Experimental particle system support
#define PARTICLE_SYSTEM 0

#include <cmath>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <iostream>
#include <memory>
#include <utility>
#include <vector>

#include <fmt/ostream.h>
#include <fmt/printf.h>

#include <cel3ds/3dsmodel.h>
#include <cel3ds/3dsread.h>
#include <celmath/randutils.h>
#include <celmodel/material.h>
#include <celmodel/mesh.h>
#include <celmodel/model.h>
#include <celmodel/modelfile.h>
#include <celutil/filetype.h>
#include <celutil/gettext.h>
#include <celutil/logger.h>
#include <celutil/tokenizer.h>
#include "meshmanager.h"
#include "modelgeometry.h"
#include "parser.h"
#include "spheremesh.h"
#include "texmanager.h"

#if PARTICLE_SYSTEM
#include "particlesystem.h"
#include "particlesystemfile.h"
#endif

using celestia::util::GetLogger;

namespace
{

struct NoiseMeshParameters
{
    Eigen::Vector3f size;
    Eigen::Vector3f offset;
    float featureHeight;
    float octaves;
    float slices;
    float rings;
};


float
NoiseDisplacementFunc(float u, float v, void* info)
{
    float theta = u * static_cast<float>(PI) * 2;
    float phi = (v - 0.5f) * static_cast<float>(PI);

    // assert(info != nullptr);
    auto* params = static_cast<NoiseMeshParameters*>(info);

    Eigen::Vector3f p = Eigen::Vector3f(std::cos(phi) * std::cos(theta),
                                        std::sin(phi),
                                        std::cos(phi) * std::sin(theta))
                        + params->offset;
    return celmath::fractalsum(p, params->octaves) * params->featureHeight;
}


// TODO: The Celestia mesh format is deprecated
std::unique_ptr<cmod::Model>
LoadCelestiaMesh(const fs::path& filename)
{
    std::ifstream meshFile(filename, std::ios::in);
    if (!meshFile.good())
    {
        GetLogger()->error("Error opening mesh file: {}\n", filename);
        return nullptr;
    }

    Tokenizer tokenizer(&meshFile);
    Parser parser(&tokenizer);

    if (tokenizer.nextToken() != Tokenizer::TokenName)
    {
        GetLogger()->error("Mesh file {} is invalid.\n", filename);
        return nullptr;
    }

    if (tokenizer.getStringValue() != "SphereDisplacementMesh")
    {
        GetLogger()->error("{}: Unrecognized mesh type {}.\n",
                filename, tokenizer.getStringValue());
        return nullptr;
    }

    Value* meshDefValue = parser.readValue();
    if (meshDefValue == nullptr)
    {
        GetLogger()->error("{}: Bad mesh file.\n", filename);
        return nullptr;
    }

    if (meshDefValue->getType() != Value::HashType)
    {
        GetLogger()->error("{}: Bad mesh file.\n", filename);
        delete meshDefValue;
        return nullptr;
    }

    Hash* meshDef = meshDefValue->getHash();

    NoiseMeshParameters params{};

    params.size = Eigen::Vector3f::Ones();
    params.offset = Eigen::Vector3f::Constant(10.0f);
    params.featureHeight = 0.0f;
    params.octaves = 1;
    params.slices = 20;
    params.rings = 20;

    meshDef->getVector("Size", params.size);
    meshDef->getVector("NoiseOffset", params.offset);
    meshDef->getNumber("FeatureHeight", params.featureHeight);
    meshDef->getNumber("Octaves", params.octaves);
    meshDef->getNumber("Slices", params.slices);
    meshDef->getNumber("Rings", params.rings);

    delete meshDefValue;

    auto model = std::make_unique<cmod::Model>();
    SphereMesh sphereMesh(params.size,
                          static_cast<int>(params.rings),
                          static_cast<int>(params.slices),
                          NoiseDisplacementFunc,
                          (void*) &params);
    model->addMesh(sphereMesh.convertToMesh());

    return model;
}


cmod::Mesh
ConvertTriangleMesh(const M3DTriangleMesh& mesh,
                    const M3DScene& scene)
{
    int nFaces     = mesh.getFaceCount();
    int nVertices  = mesh.getVertexCount();
    int nTexCoords = mesh.getTexCoordCount();

    // Texture coordinates are optional. Check for tex coord count >= nVertices because some
    // convertors generate extra texture coordinates.
    bool hasTextureCoords = nTexCoords >= nVertices;

    // Create the attribute set. Always include positions and normals, texture coords
    // are optional.
    std::vector<cmod::VertexAttribute> attributes;
    attributes.reserve(3);
    std::uint32_t vertexSize = 0;

    // Position attribute are required
    attributes.emplace_back(cmod::VertexAttributeSemantic::Position,
                            cmod::VertexAttributeFormat::Float3,
                            vertexSize);
    vertexSize += 3;

    // Normals are always generated
    attributes.emplace_back(cmod::VertexAttributeSemantic::Normal,
                            cmod::VertexAttributeFormat::Float3,
                            vertexSize);
    vertexSize += 3;

    if (hasTextureCoords)
    {
        attributes.emplace_back(cmod::VertexAttributeSemantic::Texture0,
                                cmod::VertexAttributeFormat::Float2,
                                vertexSize);
        vertexSize += 2;
    }

    // bool smooth = (mesh.getSmoothingGroupCount() == nFaces);

    std::vector<Eigen::Vector3f> faceNormals;
    faceNormals.reserve(nFaces);
    std::vector<Eigen::Vector3f> vertexNormals;
    vertexNormals.reserve(nFaces * 3);
    std::vector<int> faceCounts(nVertices, 0);
    std::vector<std::vector<int>> vertexFaces(nVertices);

    // generate face normals
    for (int i = 0; i < nFaces; i++)
    {
        std::uint16_t v0, v1, v2;
        mesh.getFace(i, v0, v1, v2);

        faceCounts[v0]++;
        faceCounts[v1]++;
        faceCounts[v2]++;

        Eigen::Vector3f p0 = mesh.getVertex(v0);
        Eigen::Vector3f p1 = mesh.getVertex(v1);
        Eigen::Vector3f p2 = mesh.getVertex(v2);
        faceNormals.push_back((p1 - p0).cross(p2 - p1).normalized());
    }

#if 0
    if (!smooth)
    {
        for (int i = 0; i < nFaces; i++)
        {
            vertexNormals.push_back(faceNormals[i]);
            vertexNormals.push_back(faceNormals[i]);
            vertexNormals.push_back(faceNormals[i]);
        }
    }
    else
#endif
    {
        // allocate space for vertex face indices
        for (int i = 0; i < nVertices; i++)
        {
            vertexFaces[i].resize(faceCounts[i] + 1);
            vertexFaces[i][0] = faceCounts[i];
        }

        for (int i = 0; i < nFaces; i++)
        {
            std::uint16_t v0, v1, v2;
            mesh.getFace(i, v0, v1, v2);
            vertexFaces[v0][faceCounts[v0]--] = i;
            vertexFaces[v1][faceCounts[v1]--] = i;
            vertexFaces[v2][faceCounts[v2]--] = i;
        }

        // average face normals to compute the vertex normals
        for (int i = 0; i < nFaces; i++)
        {
            std::uint16_t v0, v1, v2;
            mesh.getFace(i, v0, v1, v2);
            // uint32_t smoothingGroups = mesh.getSmoothingGroups(i);

            Eigen::Vector3f v = Eigen::Vector3f::Zero();
            for (int j = 1; j <= vertexFaces[v0][0]; j++)
            {
                int k = vertexFaces[v0][j];
                // if (k == i || (smoothingGroups & mesh.getSmoothingGroups(k)) != 0)
                if (faceNormals[i].dot(faceNormals[k]) > 0.5f)
                    v += faceNormals[k];
            }
            vertexNormals.push_back(v.normalized());

            v = Eigen::Vector3f::Zero();
            for (int j = 1; j <= vertexFaces[v1][0]; j++)
            {
                int k = vertexFaces[v1][j];
                // if (k == i || (smoothingGroups & mesh.getSmoothingGroups(k)) != 0)
                if (faceNormals[i].dot(faceNormals[k]) > 0.5f)
                    v += faceNormals[k];
            }
            vertexNormals.push_back(v.normalized());

            v = Eigen::Vector3f::Zero();
            for (int j = 1; j <= vertexFaces[v2][0]; j++)
            {
                int k = vertexFaces[v2][j];
                // if (k == i || (smoothingGroups & mesh.getSmoothingGroups(k)) != 0)
                if (faceNormals[i].dot(faceNormals[k]) > 0.5f)
                    v += faceNormals[k];
            }
            vertexNormals.push_back(v.normalized());
        }
    }

    // Create the vertex data
    static_assert(sizeof(float) == sizeof(cmod::VWord), "Float does not match vertex data word size");
    std::vector<cmod::VWord> vertexData(nFaces * 3 * vertexSize);

    for (int i = 0; i < nFaces; i++)
    {
        std::uint16_t triVert[3];
        mesh.getFace(i, triVert[0], triVert[1], triVert[2]);

        for (unsigned int j = 0; j < 3; j++)
        {
            Eigen::Vector3f position = mesh.getVertex(triVert[j]);
            Eigen::Vector3f normal = vertexNormals[i * 3 + j];

            int dataOffset = (i * 3 + j) * vertexSize;
            std::memcpy(vertexData.data() + dataOffset, position.data(), sizeof(float) * 3);
            std::memcpy(vertexData.data() + dataOffset + 3, normal.data(), sizeof(float) * 3);
            if (hasTextureCoords)
            {
                Eigen::Vector2f texCoord = mesh.getTexCoord(triVert[j]);
                std::memcpy(vertexData.data() + dataOffset + 6, texCoord.data(), sizeof(float) * 2);
            }
        }
    }

    // Create the mesh
    cmod::Mesh newMesh;
    newMesh.setVertexDescription(cmod::VertexDescription(std::move(attributes)));
    newMesh.setVertices(nFaces * 3, std::move(vertexData));

    for (uint32_t i = 0; i < mesh.getMeshMaterialGroupCount(); ++i)
    {
        const M3DMeshMaterialGroup* matGroup = mesh.getMeshMaterialGroup(i);

        // Vertex lists are not indexed, so the conversion to an indexed format is
        // trivial (although much space is wasted storing unnecessary indices.)
        std::uint32_t nMatGroupFaces = matGroup->faces.size();

        std::vector<cmod::Index32> indices;
        indices.reserve(nMatGroupFaces * 3);
        for (std::uint32_t j = 0; j < nMatGroupFaces; ++j)
        {
            std::uint16_t faceIndex = matGroup->faces[j];
            indices.push_back(faceIndex * 3 + 0);
            indices.push_back(faceIndex * 3 + 1);
            indices.push_back(faceIndex * 3 + 2);
        }

        // Lookup the material index
        std::uint32_t materialIndex = 0;
        for (std::uint32_t j = 0; j < scene.getMaterialCount(); ++j)
        {
            if (matGroup->materialName == scene.getMaterial(j)->getName())
            {
                materialIndex = j;
                break;
            }
        }

        newMesh.addGroup(cmod::PrimitiveGroupType::TriList, materialIndex, std::move(indices));
    }

    return newMesh;
}


std::unique_ptr<cmod::Model>
Convert3DSModel(const M3DScene& scene, const fs::path& texPath)
{
    auto model = std::make_unique<cmod::Model>();

    // Convert the materials
    for (std::uint32_t i = 0; i < scene.getMaterialCount(); i++)
    {
        const M3DMaterial* material = scene.getMaterial(i);
        cmod::Material newMaterial;

        M3DColor diffuse = material->getDiffuseColor();
        newMaterial.diffuse = cmod::Color(diffuse.red, diffuse.green, diffuse.blue);
        newMaterial.opacity = material->getOpacity();

        M3DColor specular = material->getSpecularColor();
        newMaterial.specular = cmod::Color(specular.red, specular.green, specular.blue);

        float shininess = material->getShininess();

        // Map the 3DS file's shininess from percentage (0-100) to
        // range that OpenGL uses for the specular exponent. The
        // current equation is just a guess at the mapping that
        // 3DS actually uses.
        newMaterial.specularPower = std::pow(2.0f, 1.0f + 0.1f * shininess);
        if (newMaterial.specularPower > 128.0f)
            newMaterial.specularPower = 128.0f;

        if (!material->getTextureMap().empty())
        {
            ResourceHandle tex = GetTextureManager()->getHandle(TextureInfo(material->getTextureMap(), texPath, TextureInfo::WrapTexture));
            newMaterial.setMap(cmod::TextureSemantic::DiffuseMap, tex);
        }

        model->addMaterial(std::move(newMaterial));
    }

    // Convert all models in the scene. Some confusing terminology: a 3ds 'scene' is the same
    // as a Celestia model, and a 3ds 'model' is the same as a Celestia mesh.
    for (std::uint32_t i = 0; i < scene.getModelCount(); i++)
    {
        const M3DModel* model3ds = scene.getModel(i);
        if (model3ds)
        {
            for (unsigned int j = 0; j < model3ds->getTriMeshCount(); j++)
            {
                const M3DTriangleMesh* mesh = model3ds->getTriMesh(j);
                if (mesh)
                {
                    model->addMesh(ConvertTriangleMesh(*mesh, scene));
                }
            }
        }
    }

    return model;
}

constexpr const fs::path::value_type UniqueSuffixChar = '!';

} // end unnamed namespace




GeometryManager*
GetGeometryManager()
{
    static GeometryManager geometryManager("models");
    return &geometryManager;
}


fs::path
GeometryInfo::resolve(const fs::path& baseDir)
{
    // Ensure that models with different centers get resolved to different objects by
    // adding a 'uniquifying' suffix to the filename that encodes the center value.
    // This suffix is stripped before the file is actually loaded.
    fs::path::string_type uniquifyingSuffix;
    fs::path::string_type format;
#ifdef _WIN32
    format = L"%c%f,%f,%f,%f,%d";
#else
    format = "%c%f,%f,%f,%f,%d";
#endif
    uniquifyingSuffix = fmt::sprintf(format, UniqueSuffixChar, center.x(), center.y(), center.z(), scale, (int) isNormalized);

    if (!path.empty())
    {
        fs::path filename = path / "models" / source;
        std::ifstream in(filename);
        if (in.good())
        {
            resolvedToPath = true;
            return filename += uniquifyingSuffix;
        }
    }

    return (baseDir / source) += uniquifyingSuffix;
}


Geometry*
GeometryInfo::load(const fs::path& resolvedFilename)
{
    // Strip off the uniquifying suffix
    fs::path::string_type::size_type uniquifyingSuffixStart = resolvedFilename.native().rfind(UniqueSuffixChar);
    fs::path filename = resolvedFilename.native().substr(0, uniquifyingSuffixStart);

    std::clog << fmt::sprintf(_("Loading model: %s\n"), filename);
    std::unique_ptr<cmod::Model> model = nullptr;
    ContentType fileType = DetermineFileType(filename);

    if (fileType == Content_3DStudio)
    {
        std::unique_ptr<M3DScene> scene = Read3DSFile(filename);
        if (scene != nullptr)
        {
            if (resolvedToPath)
                model = Convert3DSModel(*scene, path);
            else
                model = Convert3DSModel(*scene, "");

            if (isNormalized)
                model->normalize(center);
            else
                model->transform(center, scale);
        }
    }
    else if (fileType == Content_CelestiaModel)
    {
        std::ifstream in(filename, std::ios::binary);
        if (in.good())
        {
            model = cmod::LoadModel(
                in,
                [&](const fs::path& name)
                {
                    return GetTextureManager()->getHandle(TextureInfo(name, path, TextureInfo::WrapTexture));
                });
            if (model != nullptr)
            {
                if (isNormalized)
                    model->normalize(center);
                else
                    model->transform(center, scale);
            }
        }
    }
    else if (fileType == Content_CelestiaMesh)
    {
        model = LoadCelestiaMesh(filename);
        if (model != nullptr)
        {
            if (isNormalized)
                model->normalize(center);
            else
                model->transform(center, scale);
        }
    }
#if PARTICLE_SYSTEM
    else if (fileType == Content_CelestiaParticleSystem)
    {
        ifstream in(filename);
        if (in.good())
        {
            return LoadParticleSystem(in, path);
        }
    }
#endif

    // Condition the model for optimal rendering
    if (model != nullptr)
    {
        // Many models tend to have a lot of duplicate materials; eliminate
        // them, since unnecessarily setting material parameters can adversely
        // impact rendering performance. Ideally uniquification of materials
        // would be performed just once when the model was created, but
        // that's not the case.
        std::uint32_t originalMaterialCount = model->getMaterialCount();
        model->uniquifyMaterials();

        // Sort the submeshes roughly by opacity.  This will eliminate a
        // good number of the errors caused when translucent triangles are
        // rendered before geometry that they cover.
        model->sortMeshes(cmod::Model::OpacityComparator());

        model->determineOpacity();

        // Display some statics for the model
        std::clog << fmt::sprintf(
                        _("   Model statistics: %u vertices, %u primitives, %u materials (%u unique)\n"),
                        model->getVertexCount(),
                        model->getPrimitiveCount(),
                        originalMaterialCount,
                        model->getMaterialCount());

        return new ModelGeometry(std::move(model));
    }
    else
    {
        std::clog << fmt::sprintf(_("Error loading model '%s'\n"), filename);
        return nullptr;
    }
}
