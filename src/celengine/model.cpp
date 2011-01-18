// model.cpp
//
// Copyright (C) 2004-2006, Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include <algorithm>
#include <cassert>
#include <functional>
#include "model.h"
#include "rendcontext.h"

using namespace std;


#if 0
static GLenum GLPrimitiveModes[MaxPrimitiveType] =
{
    GL_TRIANGLES,
    GL_TRIANGLE_STRIP,
    GL_TRIANGLE_FAN,
    GL_LINES,
    GL_LINE_STRIP,
    GL_POINTS
};
#endif

#if 0
static size_t VertexAttributeFormatSizes[Mesh::FormatMax] =
{
     4,  // Float1
     8,  // Float2
     12, // Float3
     16, // Float4,
     4,  // UByte4
};
#endif


Model::Model() :
    opaque(true),
    normalized(false)
{
    for (int i = 0; i < Mesh::TextureSemanticMax; i++)
        textureUsage[i] = false;
}


Model::~Model()
{
    {
        for (vector<Mesh*>::iterator iter = meshes.begin();
             iter != meshes.end(); iter++)
            delete *iter;
    }

#if 0
    {
        for (vector<const Mesh::Material*>::iterator iter = materials.begin();
             iter != materials.end(); iter++)
            delete *iter;
    }
#endif
}


const Mesh::Material*
Model::getMaterial(uint32 index) const
{
    if (index < materials.size())
        return materials[index];
    else
        return NULL;
}


uint32
Model::addMaterial(const Mesh::Material* m)
{
    // Update the texture map usage information for the model.  Since
    // the material being added isn't necessarily used by a mesh within
    // the model, we could potentially end up with false positives--this
    // won't cause any rendering troubles, but could hurt performance
    // if it forces multipass rendering when it's not required.
    for (int i = 0; i < Mesh::TextureSemanticMax; i++)
    {
        if (m->maps[i] != InvalidResource)
            textureUsage[i] = true;
    }

    materials.push_back(m);
    return materials.size();
}


uint32
Model::getMaterialCount() const
{
    return materials.size();
}


uint32
Model::getVertexCount() const
{
    uint32 count = 0;

    for (vector<Mesh*>::const_iterator iter = meshes.begin();
         iter != meshes.end(); iter++)
    {
        count += (*iter)->getVertexCount();
    }

    return count;
}


uint32
Model::getPrimitiveCount() const
{
    uint32 count = 0;

    for (vector<Mesh*>::const_iterator iter = meshes.begin();
         iter != meshes.end(); iter++)
    {
        count += (*iter)->getPrimitiveCount();
    }

    return count;
}


Mesh*
Model::getMesh(uint32 index) const
{
    if (index < meshes.size())
        return meshes[index];
    else
        return NULL;
}


uint32
Model::addMesh(Mesh* m)
{
    meshes.push_back(m);
    return meshes.size();
}


bool
Model::pick(const Ray3d& r, double& distance) const
{
    double maxDistance = 1.0e30;
    double closest = maxDistance;

    for (vector<Mesh*>::const_iterator iter = meshes.begin();
         iter != meshes.end(); iter++)
    {
        double d = maxDistance;
        if ((*iter)->pick(r, d) && d < closest)
            closest = d;
    }

    if (closest != maxDistance)
    {
        distance = closest;
        return true;
    }
    else
    {
        return false;
    }
}


/*! Render the model; the time parameter is ignored right now
 *  since this class doesn't currently support animation.
 */
void
Model::render(RenderContext& rc, double /* t */)
{
    for (vector<Mesh*>::const_iterator iter = meshes.begin();
         iter != meshes.end(); iter++)
    {
        (*iter)->render(materials, rc);
    }
}


/*! Translate and scale a model. The transformation applied to
 *  each vertex in the model is:
 *     v' = (v + translation) * scale
 */
void
Model::transform(const Vec3f& translation, float scale)
{
    for (vector<Mesh*>::const_iterator iter = meshes.begin(); iter != meshes.end(); iter++)
        (*iter)->transform(translation, scale);
}


void
Model::normalize(const Vec3f& centerOffset)
{
    AxisAlignedBox bbox;

    for (vector<Mesh*>::const_iterator iter = meshes.begin(); iter != meshes.end(); iter++)
        bbox.include((*iter)->getBoundingBox());

    Point3f center = bbox.getCenter() + centerOffset;
    Vec3f extents = bbox.getExtents();
    float maxExtent = extents.x;
    if (extents.y > maxExtent)
        maxExtent = extents.y;
    if (extents.z > maxExtent)
        maxExtent = extents.z;
    
    // clog << "Extents: " << extents.x << ", " << extents.y << ", " << extents.z << endl;

    transform(Point3f(0, 0, 0) - center, 2.0f / maxExtent);

    normalized = true;
}


static bool
operator<(const Color& c0, const Color& c1)
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
operator<(const Mesh::Material& m0, const Mesh::Material& m1)
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

    for (unsigned int i = 0; i < Mesh::TextureSemanticMax; i++)
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
    const Mesh::Material* material;
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

    vector<const Mesh::Material*> uniqueMaterials;
    vector<uint32> materialMap(materials.size());
    vector<uint32> duplicateMaterials;

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

    vector<uint32>::const_iterator dupIter;
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
            materials[i]->blend == Mesh::AdditiveBlend)
        {
            opaque = false;
            return;
        }
    }

    opaque = true;
}


bool
Model::usesTextureType(Mesh::TextureSemantic t) const
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
static uint32
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
