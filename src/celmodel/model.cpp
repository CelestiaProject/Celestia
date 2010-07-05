// model.cpp
//
// Copyright (C) 2004-2010, Celestia Development Team
// Original version by Chris Laurel <claurel@gmail.com>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include "model.h"
#include <cassert>
#include <functional>
#include <algorithm>

using namespace cmod;
using namespace Eigen;
using namespace std;


Model::Model() :
    opaque(true),
    normalized(false)
{
    for (int i = 0; i < Material::TextureSemanticMax; i++)
    {
        textureUsage[i] = false;
    }
}


Model::~Model()
{
    for (vector<Mesh*>::iterator iter = meshes.begin(); iter != meshes.end(); iter++)
    {
        delete *iter;
    }
}


/*! Return the material with the specified index, or NULL if
 *  the index is out of range. The returned material pointer
 *  is const.
 */
const Material*
Model::getMaterial(unsigned int index) const
{
    if (index < materials.size())
        return materials[index];
    else
        return NULL;
}


/*! Add a new material to the model's material library; the
 *  return value is the number of materials in the model.
 */
unsigned int
Model::addMaterial(const Material* m)
{
    // Update the texture map usage information for the model.  Since
    // the material being added isn't necessarily used by a mesh within
    // the model, we could potentially end up with false positives--this
    // won't cause any rendering troubles, but could hurt performance
    // if it forces multipass rendering when it's not required.
    for (int i = 0; i < Material::TextureSemanticMax; i++)
    {
        if (m->maps[i])
        {
            textureUsage[i] = true;
        }
    }

    materials.push_back(m);
    return materials.size();
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

    for (vector<Mesh*>::const_iterator iter = meshes.begin();
         iter != meshes.end(); iter++)
    {
        count += (*iter)->getVertexCount();
    }

    return count;
}


unsigned int
Model::getPrimitiveCount() const
{
    unsigned int count = 0;

    for (vector<Mesh*>::const_iterator iter = meshes.begin();
         iter != meshes.end(); iter++)
    {
        count += (*iter)->getPrimitiveCount();
    }

    return count;
}


unsigned int
Model::getMeshCount() const
{
    return meshes.size();
}


Mesh*
Model::getMesh(unsigned int index) const
{
    if (index < meshes.size())
        return meshes[index];
    else
        return NULL;
}


unsigned int
Model::addMesh(Mesh* m)
{
    meshes.push_back(m);
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

    for (vector<Mesh*>::const_iterator iter = meshes.begin();
         iter != meshes.end(); iter++)
    {
        Mesh::PickResult result;
        if ((*iter)->pick(rayOrigin, rayDirection, &result))
        {
            if (result.distance < closest)
            {
                closestResult = result;
                closestResult.mesh = *iter;
                closest = result.distance;
            }
        }
    }

    if (closest != maxDistance)
    {
        if (result)
        {
            *result = closestResult;
        }
        return true;
    }
    else
    {
        return false;
    }

}


bool
Model::pick(const Vector3d& rayOrigin, const Vector3d& rayDirection, double& distance) const
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
Model::transform(const Vector3f& translation, float scale)
{
    for (vector<Mesh*>::const_iterator iter = meshes.begin(); iter != meshes.end(); iter++)
        (*iter)->transform(translation, scale);
}


void
Model::normalize(const Vector3f& centerOffset)
{
    AlignedBox<float, 3> bbox;

    for (vector<Mesh*>::const_iterator iter = meshes.begin(); iter != meshes.end(); iter++)
        bbox.extend((*iter)->getBoundingBox());

    Vector3f center = (bbox.min() + bbox.max()) * 0.5f + centerOffset;
    Vector3f extents = bbox.max() - bbox.min();
    float maxExtent = extents.maxCoeff();

    transform(-center, 2.0f / maxExtent);

    normalized = true;
}


static bool
operator<(const Material::Color& c0, const Material::Color& c1)
{
    if (c0.red() < c1.red())
        return true;
    else if (c0.red() > c1.red())
        return false;

    if (c0.green() < c1.green())
        return true;
    else if (c0.green() > c1.green())
        return false;

    return c0.blue() < c1.blue();
}


// Define an ordering for materials; required for elimination of duplicate
// materials.
static bool
operator<(const Material& m0, const Material& m1)
{
    // Checking opacity first and doing it backwards is deliberate. It means
    // that after sorting, translucent materials will end up with higher
    // material indices than opaque ones. Ultimately, after sorting
    // mesh primitive groups by material, translucent groups will end up
    // rendered after opaque ones.
    if (m0.opacity < m1.opacity)
        return true;
    else if (m0.opacity > m1.opacity)
        return false;

    // Reverse sense of comparison here--additive blending is 1, normal
    // blending is 0, and we'd prefer to render additively blended submeshes
    // last.
    if (m0.blend > m1.blend)
        return true;
    else if (m0.blend < m1.blend)
        return false;

    if (m0.diffuse < m1.diffuse)
        return true;
    else if (m1.diffuse < m0.diffuse)
        return false;

    if (m0.emissive < m1.emissive)
        return true;
    else if (m1.emissive < m0.emissive)
        return false;

    if (m0.specular < m1.specular)
        return true;
    else if (m1.specular < m0.specular)
        return false;

    if (m0.specularPower < m1.specularPower)
        return true;
    else if (m0.specularPower > m1.specularPower)
        return false;

    for (unsigned int i = 0; i < Material::TextureSemanticMax; i++)
    {
        if (m0.maps[i] < m1.maps[i])
            return true;
        else if (m0.maps[i] > m1.maps[i])
            return false;
    }

    // Materials are identical
    return false;
}


// Simple struct that pairs an index with a material; the index is used to
// keep track of the original material index after sorting.
struct IndexedMaterial
{
    int originalIndex;
    const Material* material;
};


static bool
operator<(const IndexedMaterial& im0, const IndexedMaterial& im1)
{
    return *(im0.material) < *(im1.material);
}


static bool
operator!=(const IndexedMaterial& im0, const IndexedMaterial& im1)
{
    return im0 < im1 || im1 < im0;
}


void
Model::uniquifyMaterials()
{
    // No work to do if there's just a single material
    if (materials.size() <= 1)
        return;

    // Create an array of materials with the indices attached
    vector<IndexedMaterial> indexedMaterials;
    unsigned int i;
    for (i = 0; i < materials.size(); i++)
    {
        IndexedMaterial im;
        im.originalIndex = i;
        im.material = materials[i];
        indexedMaterials.push_back(im);
    }

    // Sort the indexed materials so that we can uniquify them
    sort(indexedMaterials.begin(), indexedMaterials.end());

    vector<const Material*> uniqueMaterials;
    vector<unsigned int> materialMap(materials.size());
    vector<unsigned int> duplicateMaterials;

    // From the sorted material list construct the list of unique materials
    // and a map to convert old material indices into indices that can be
    // used with the uniquified material list.
    unsigned int uniqueMaterialCount = 0;
    for (i = 0; i < indexedMaterials.size(); i++)
    {
        if (i == 0 || indexedMaterials[i] != indexedMaterials[i - 1])
        {
            uniqueMaterialCount++;
            uniqueMaterials.push_back(indexedMaterials[i].material);
        }
        else
        {
            duplicateMaterials.push_back(i);
        }
        materialMap[indexedMaterials[i].originalIndex] = uniqueMaterialCount - 1;
    }

    // Remap all the material indices in the model. Even if no materials have
    // been eliminated we've still sorted them by opacity, which is useful
    // when reordering meshes so that translucent ones are rendered last.
    for (vector<Mesh*>::iterator iter = meshes.begin();
         iter != meshes.end(); iter++)
    {
        (*iter)->remapMaterials(materialMap);
    }

    vector<unsigned int>::const_iterator dupIter;
    for (dupIter = duplicateMaterials.begin();
         dupIter != duplicateMaterials.end(); ++dupIter)
    {
        delete indexedMaterials[*dupIter].material;
    }

    materials = uniqueMaterials;
}


void
Model::determineOpacity()
{
    for (unsigned int i = 0; i < materials.size(); i++)
    {
        if ((materials[i]->opacity > 0.01f && materials[i]->opacity < 1.0f) ||
            materials[i]->blend == Material::AdditiveBlend)
        {
            opaque = false;
            return;
        }
    }

    opaque = true;
}


bool
Model::usesTextureType(Material::TextureSemantic t) const
{
    return textureUsage[static_cast<int>(t)];
}


class MeshComparatorAdapter : public std::binary_function<const Mesh*, const Mesh*, bool>
{
public:
    MeshComparatorAdapter(const Model::MeshComparator& c) :
        comparator(c)
    {
    }

    bool operator()(const Mesh* a, const Mesh* b) const
    {
        return comparator(*a, *b);
    }

private:
    const Model::MeshComparator& comparator;
};


Model::OpacityComparator::OpacityComparator()
{
}


// Look at the material used by last primitive group in the mesh for the
// opacity of the whole model.  This is a very crude way to check the opacity
// of a mesh and misses many cases.
static unsigned int
getMeshMaterialIndex(const Mesh& mesh)
{
    if (mesh.getGroupCount() == 0)
        return 0;
    else
        return mesh.getGroup(mesh.getGroupCount() - 1)->materialIndex;
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
    for (vector<Mesh*>::const_iterator iter = meshes.begin();
         iter != meshes.end(); iter++)
    {
        (*iter)->aggregateByMaterial();
    }

    // Sort the meshes so that completely opaque ones are first
    sort(meshes.begin(), meshes.end(), MeshComparatorAdapter(comparator));
}
