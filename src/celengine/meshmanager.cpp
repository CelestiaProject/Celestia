// meshmanager.cpp
//
// Copyright (C) 2001-2010, Celestia Development Team
// Original version by Chris Laurel <claurel@gmail.com>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include "meshmanager.h"

#include <cstdint>
#include <cstring>
#include <fstream>
#include <ios>
#include <utility>
#include <vector>

#include <cel3ds/3dsmodel.h>
#include <cel3ds/3dsread.h>
#include <celmodel/model.h>
#include <celmodel/modelfile.h>
#include <celutil/filetype.h>
#include <celutil/gettext.h>
#include <celutil/logger.h>
#include <celutil/tokenizer.h>
#include "hash.h"
#include "modelgeometry.h"
#include "parser.h"
#include "spheremesh.h"
#include "texmanager.h"
#include "value.h"


using celestia::util::GetLogger;

namespace
{

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

    tokenizer.nextToken();
    if (auto tokenValue = tokenizer.getNameValue(); !tokenValue.has_value())
    {
        GetLogger()->error("Mesh file {} is invalid.\n", filename);
        return nullptr;
    }
    else if (*tokenValue != "SphereDisplacementMesh")
    {
        GetLogger()->error("{}: Unrecognized mesh type {}.\n",
                           filename, *tokenValue);
        return nullptr;
    }

    const Value meshDefValue = parser.readValue();
    const Hash* meshDef = meshDefValue.getHash();
    if (meshDef == nullptr)
    {
        GetLogger()->error("{}: Bad mesh file.\n", filename);
        return nullptr;
    }

    SphereMeshParameters params{};

    params.size = meshDef->getVector3<float>("Size").value_or(Eigen::Vector3f::Ones());
    params.offset = meshDef->getVector3<float>("NoiseOffset").value_or(Eigen::Vector3f::Constant(10.0f));
    params.featureHeight = meshDef->getNumber<float>("FeatureHeight").value_or(0.0f);
    params.octaves = meshDef->getNumber<float>("Octaves").value_or(1.0f);
    params.slices = meshDef->getNumber<float>("Slices").value_or(20.0f);
    params.rings = meshDef->getNumber<float>("Rings").value_or(20.0f);

    auto model = std::make_unique<cmod::Model>();
    SphereMesh sphereMesh(params.size,
                          static_cast<int>(params.rings),
                          static_cast<int>(params.slices),
                          params);
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


std::unique_ptr<cmod::Model>
Load3DSModel(const GeometryInfo::ResourceKey& key, const fs::path& path)
{
    std::unique_ptr<M3DScene> scene = Read3DSFile(key.resolvedPath);
    if (scene == nullptr)
        return nullptr;

    std::unique_ptr<cmod::Model> model = Convert3DSModel(*scene, key.resolvedToPath ? path : fs::path());

    if (key.isNormalized)
        model->normalize(key.center);
    else
        model->transform(key.center, key.scale);

    return model;
}


std::unique_ptr<cmod::Model>
LoadCMODModel(const GeometryInfo::ResourceKey& key, const fs::path& path)
{
    std::ifstream in(key.resolvedPath, std::ios::binary);
    if (!in.good())
        return nullptr;

    std::unique_ptr<cmod::Model> model = cmod::LoadModel(
        in,
        [&](const fs::path& name)
        {
            return GetTextureManager()->getHandle(TextureInfo(name, path, TextureInfo::WrapTexture));
        });

    if (model == nullptr)
        return nullptr;

    if (key.isNormalized)
        model->normalize(key.center);
    else
        model->transform(key.center, key.scale);

    return model;
}


std::unique_ptr<cmod::Model>
LoadCMSModel(const GeometryInfo::ResourceKey& key)
{
    std::unique_ptr<cmod::Model> model = LoadCelestiaMesh(key.resolvedPath);
    if (model == nullptr)
        return nullptr;

    if (key.isNormalized)
        model->normalize(key.center);
    else
        model->transform(key.center, key.scale);

    return model;
}

} // end unnamed namespace


GeometryManager*
GetGeometryManager()
{
    static GeometryManager* geometryManager = nullptr;
    if (geometryManager == nullptr)
        geometryManager = std::make_unique<GeometryManager>("models").release();
    return geometryManager;
}


GeometryInfo::ResourceKey
GeometryInfo::resolve(const fs::path& baseDir) const
{
    if (!path.empty())
    {
        fs::path filename = path / "models" / source;
        std::ifstream in(filename);
        if (in.good())
        {
            return ResourceKey(std::move(filename), center, scale, isNormalized, true);
        }
    }

    return ResourceKey(baseDir / source, center, scale, isNormalized, false);
}


std::unique_ptr<Geometry>
GeometryInfo::load(const ResourceKey& key) const
{
    GetLogger()->info(_("Loading model: {}\n"), key.resolvedPath);
    std::unique_ptr<cmod::Model> model = nullptr;

    switch (ContentType fileType = DetermineFileType(key.resolvedPath); fileType)
    {
    case Content_3DStudio:
        model = Load3DSModel(key, path);
        break;
    case Content_CelestiaModel:
        model = LoadCMODModel(key, path);
        break;
    case Content_CelestiaMesh:
        model = LoadCMSModel(key);
        break;
    default:
        GetLogger()->error(_("Unknown model format '{}'\n"), key.resolvedPath);
        return nullptr;
    }

    if (model == nullptr)
    {
        GetLogger()->error(_("Error loading model '{}'\n"), key.resolvedPath);
        return nullptr;
    }

    // Condition the model for optimal rendering
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
    GetLogger()->verbose(_("   Model statistics: {} vertices, {} primitives, {} materials ({} unique)\n"),
                         model->getVertexCount(),
                         model->getPrimitiveCount(),
                         originalMaterialCount,
                         model->getMaterialCount());

    return std::make_unique<ModelGeometry>(std::move(model));
}
