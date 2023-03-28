// model.cpp
//
// Copyright (C) 2004-2010, Celestia Development Team
// Original version by Chris Laurel <claurel@gmail.com>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include <algorithm>
#include <functional>
#include <numeric>
#include <utility>

#include <Eigen/Geometry>

#include <celutil/logger.h>

#include "model.h"

using celestia::util::GetLogger;

namespace cmod
{
namespace
{

// Look at the material used by last primitive group in the mesh for the
// opacity of the whole model.  This is a very crude way to check the opacity
// of a mesh and misses many cases.
unsigned int
getMeshMaterialIndex(const Mesh& mesh)
{
    if (mesh.getGroupCount() == 0)
        return 0;

    return mesh.getGroup(mesh.getGroupCount() - 1)->materialIndex;
}

} // end unnamed namespace


Model::Model()
{
    textureUsage.fill(false);
}


/*! Return the material with the specified index, or nullptr if
 *  the index is out of range. The returned material pointer
 *  is const.
 */
const Material*
Model::getMaterial(unsigned int index) const
{
    if (index < materials.size())
        return &materials[index];
    else
        return nullptr;
}


/*! Add a new material to the model's material library; the
 *  return value is the number of materials in the model.
 */
unsigned int
Model::addMaterial(Material&& m)
{
    // Update the texture map usage information for the model.  Since
    // the material being added isn't necessarily used by a mesh within
    // the model, we could potentially end up with false positives--this
    // won't cause any rendering troubles, but could hurt performance
    // if it forces multipass rendering when it's not required.
    for (int i = 0; i < static_cast<int>(TextureSemantic::TextureSemanticMax); ++i)
    {
        if (m.maps[i] != InvalidResource)
        {
            textureUsage[i] = true;
        }
    }

    materials.push_back(std::move(m));
    return materials.size();
}


bool
Model::setMaterial(unsigned int index, Material&& m)
{
    if (index >= materials.size()) { return false; }
    materials[index] = std::move(m);

    // Regenerate the texture map usage for the model by rescanning all the meshes.
    for (int i = 0; i < static_cast<int>(TextureSemantic::TextureSemanticMax); ++i)
    {
        textureUsage[i] = std::any_of(materials.cbegin(), materials.cend(),
                                      [&](const Material& mat) { return mat.maps[i] != InvalidResource; });
    }

    return true;
}


unsigned int
Model::getMaterialCount() const
{
    return materials.size();
}


unsigned int
Model::getVertexCount() const
{
    unsigned int count = 0;

    for (const auto& mesh : meshes)
        count += mesh.getVertexCount();

    return count;
}


unsigned int
Model::getPrimitiveCount() const
{
    unsigned int count = 0;

    for (const auto& mesh : meshes)
        count += mesh.getPrimitiveCount();

    return count;
}


unsigned int
Model::getMeshCount() const
{
    return meshes.size();
}


Mesh*
Model::getMesh(unsigned int index)
{
    if (index < meshes.size())
        return &meshes[index];
    else
        return nullptr;
}


const Mesh*
Model::getMesh(unsigned int index) const
{
    if (index < meshes.size())
        return &meshes[index];
    else
        return nullptr;
}


unsigned int
Model::addMesh(Mesh&& m)
{
    meshes.push_back(std::move(m));
    return meshes.size();
}


bool
Model::pick(const Eigen::Vector3d& rayOrigin,
            const Eigen::Vector3d& rayDirection,
            Mesh::PickResult* result) const
{
    double maxDistance = 1.0e30;
    double closest = maxDistance;
    Mesh::PickResult closestResult;

    for (const auto& mesh : meshes)
    {
        Mesh::PickResult result;
        if (mesh.pick(rayOrigin, rayDirection, &result))
        {
            if (result.distance < closest)
            {
                closestResult = result;
                closestResult.mesh = &mesh;
                closest = result.distance;
            }
        }
    }

    if (closest != maxDistance)
    {
        if (result != nullptr)
        {
            *result = closestResult;
        }
        return true;
    }

    return false;
}


bool
Model::pick(const Eigen::Vector3d& rayOrigin, const Eigen::Vector3d& rayDirection, double& distance) const
{
    Mesh::PickResult result;
    bool hit = pick(rayOrigin, rayDirection, &result);
    if (hit)
    {
        distance = result.distance;
    }

    return hit;
}


/*! Translate and scale a model. The transformation applied to
 *  each vertex in the model is:
 *     v' = (v + translation) * scale
 */
void
Model::transform(const Eigen::Vector3f& translation, float scale)
{
    for (auto& mesh : meshes)
        mesh.transform(translation, scale);
}


void
Model::normalize(const Eigen::Vector3f& centerOffset)
{
    Eigen::AlignedBox<float, 3> bbox;

    for (const auto& mesh : meshes)
        bbox.extend(mesh.getBoundingBox());

    Eigen::Vector3f center = (bbox.min() + bbox.max()) * 0.5f + centerOffset;
    Eigen::Vector3f extents = bbox.max() - bbox.min();
    float maxExtent = extents.maxCoeff();

    transform(-center, 2.0f / maxExtent);

    normalized = true;
}


void
Model::uniquifyMaterials()
{
    // No work to do if there's just a single material
    if (materials.size() <= 1)
        return;

    // Create an array of material indices
    std::vector<unsigned int> indices(materials.size());
    std::iota(indices.begin(), indices.end(), 0U);

    // Sort the material indices so that we can uniquify the materials
    std::sort(indices.begin(), indices.end(),
              [&](unsigned int a, unsigned int b) { return materials[a] < materials[b]; });

    // From the sorted index list construct the list of unique materials
    // and a map to convert old material indices into indices that can be
    // used with the uniquified material list.
    std::vector<unsigned int> materialMap(materials.size());

    std::vector<Material> uniqueMaterials;
    uniqueMaterials.reserve(materials.size());

    for (std::size_t i = 0; i < indices.size(); ++i)
    {
        unsigned int index = indices[i];
        if (i == 0 || materials[index] != uniqueMaterials.back())
        {
            uniqueMaterials.push_back(std::move(materials[index]));
        }

        materialMap[index] = uniqueMaterials.size() - 1;
    }

    // Remap all the material indices in the model. Even if no materials have
    // been eliminated we've still sorted them by opacity, which is useful
    // when reordering meshes so that translucent ones are rendered last.
    for (auto& mesh : meshes)
    {
        mesh.remapMaterials(materialMap);
    }

    materials = std::move(uniqueMaterials);
}


void
Model::determineOpacity()
{
    for (unsigned int i = 0; i < materials.size(); i++)
    {
        if ((materials[i].opacity > 0.01f && materials[i].opacity < 1.0f) ||
            materials[i].blend == BlendMode::AdditiveBlend)
        {
            opaque = false;
            return;
        }
    }

    opaque = true;
}


bool
Model::usesTextureType(TextureSemantic t) const
{
    return textureUsage[static_cast<std::size_t>(t)];
}


bool
Model::OpacityComparator::operator()(const Mesh& a, const Mesh& b) const
{
    // Because materials are sorted by opacity, we can just compare
    // the material index.
    return getMeshMaterialIndex(a) > getMeshMaterialIndex(b);
}


void
Model::sortMeshes(const MeshComparator& comparator)
{
    // Sort submeshes by material; if materials have been uniquified,
    // then the submeshes will be ordered so that opaque ones are first.
    for (auto& mesh : meshes)
        mesh.aggregateByMaterial();

    // Sort the meshes so that completely opaque ones are first
    std::sort(meshes.begin(), meshes.end(), std::ref(comparator));

    std::vector<Mesh> newMeshes;
    for (const auto &mesh : meshes)
    {
        if (mesh.getGroupCount() == 0)
            continue;

        if (newMeshes.empty())
        {
            newMeshes.push_back(mesh.clone());
            continue;
        }

        auto &p = newMeshes.back();
        if (!p.canMerge(mesh, materials))
        {
            newMeshes.push_back(mesh.clone());
            continue;
        }
        p.merge(mesh);
    }
    GetLogger()->info("Merged similar meshes: {} -> {}.\n", meshes.size(), newMeshes.size());

    for (auto &mesh : newMeshes)
    {
        mesh.optimize();
        mesh.rebuildIndexMetadata();
    }
    meshes = std::move(newMeshes);
}

} // end namespace cmod
