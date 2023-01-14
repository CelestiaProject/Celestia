// mesh.cpp
//
// Copyright (C) 2004-2010, the Celestia Development Team
// Original version by Chris Laurel <claurel@gmail.com>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include <algorithm>
#include <array>
#include <cstring>
#include <iterator>
#include <tuple>
#include <utility>
#include <celutil/logger.h>

#ifdef HAVE_MESHOPTIMIZER
#include <meshoptimizer.h>
#endif

#include "mesh.h"

using celestia::util::GetLogger;

namespace cmod
{
namespace
{

bool
isOpaqueMaterial(const Material &material)
{
    return (!(material.opacity > 0.01f && material.opacity < 1.0f)) &&
             material.blend != BlendMode::AdditiveBlend;
}

} // end unnamed namespace


bool operator==(const VertexAttribute& a, const VertexAttribute& b)
{
    return std::tie(a.semantic, a.format, a.offsetWords) == std::tie(b.semantic, b.format, b.offsetWords);
}


bool operator<(const VertexAttribute& a, const VertexAttribute& b)
{
    return std::tie(a.semantic, a.format, a.offsetWords) < std::tie(b.semantic, b.format, b.offsetWords);
}


VertexDescription::VertexDescription(std::vector<VertexAttribute>&& _attributes) :
    strideBytes(0),
    attributes(std::move(_attributes))
{
    for (const auto& attr : attributes)
    {
        strideBytes += VertexAttribute::getFormatSizeWords(attr.format) * sizeof(VWord);
    }

    if (!attributes.empty())
    {
        buildSemanticMap();
    }
}


// TODO: This should be called in the constructor; we should start using
// exceptions in Celestia.
bool
VertexDescription::validate() const
{
    unsigned int stride = strideBytes / sizeof(VWord);
    for (const VertexAttribute& attr : attributes)
    {
        // Validate the attribute
        if (attr.offsetWords + VertexAttribute::getFormatSizeWords(attr.format) > stride)
            return false;
        // TODO: check for repetition of attributes
        // if (vertexAttributeMap[attr->semantic].format != InvalidFormat)
        //   return false;
    }

    return true;
}


void
VertexDescription::buildSemanticMap()
{
    for (const VertexAttribute& attr : attributes)
    {
        semanticMap[static_cast<std::size_t>(attr.semantic)] = attr;
    }
}


void
VertexDescription::clearSemanticMap()
{
    for (auto& i : semanticMap)
        i = VertexAttribute();
}


bool operator==(const VertexDescription& a, const VertexDescription& b)
{
    return std::tie(a.strideBytes, a.attributes) == std::tie(b.strideBytes, b.attributes);
}


bool operator<(const VertexDescription& a, const VertexDescription& b)
{
    return std::tie(a.strideBytes, a.attributes) < std::tie(b.strideBytes, b.attributes);
}


PrimitiveGroup
PrimitiveGroup::clone() const
{
    PrimitiveGroup newGroup;
    newGroup.prim = prim;
    newGroup.materialIndex = materialIndex;
    newGroup.indices = indices;
    newGroup.indicesCount = indicesCount;
    newGroup.indicesOffset = indicesOffset;
    return newGroup;
}


unsigned int
PrimitiveGroup::getPrimitiveCount() const
{
    switch (prim)
    {
    case PrimitiveGroupType::TriList:
        return indices.size() / 3;
    case PrimitiveGroupType::TriStrip:
    case PrimitiveGroupType::TriFan:
        return indices.size() - 2;
    case PrimitiveGroupType::LineList:
        return indices.size() / 2;
    case PrimitiveGroupType::LineStrip:
        return indices.size() - 2;
    case PrimitiveGroupType::PointList:
    case PrimitiveGroupType::SpriteList:
        return indices.size();
    default:
        // Invalid value
        return 0;
    }
}


Mesh
Mesh::clone() const
{
    Mesh newMesh;
    newMesh.vertexDesc = vertexDesc.clone();
    newMesh.nVertices = nVertices;
    newMesh.vertices = vertices;
    newMesh.groups.reserve(groups.size());
    std::transform(groups.cbegin(), groups.cend(), std::back_inserter(newMesh.groups),
                   [](const PrimitiveGroup& group) { return group.clone(); });
    newMesh.name = name;
    return newMesh;
}


void
Mesh::setVertices(unsigned int _nVertices, std::vector<VWord>&& vertexData)
{
    nVertices = _nVertices;
    vertices = std::move(vertexData);
}


bool
Mesh::setVertexDescription(VertexDescription&& desc)
{
    if (!desc.validate())
        return false;

    vertexDesc = std::move(desc);
    return true;
}


const VertexDescription& Mesh::getVertexDescription() const
{
    return vertexDesc;
}


const PrimitiveGroup*
Mesh::getGroup(unsigned int index) const
{
    if (index >= groups.size())
        return nullptr;

    return &groups[index];
}


PrimitiveGroup*
Mesh::getGroup(unsigned int index)
{
    if (index >= groups.size())
        return nullptr;

    return &groups[index];
}


unsigned int
Mesh::addGroup(PrimitiveGroup&& group)
{
    groups.push_back(std::move(group));
    return groups.size();
}


unsigned int
Mesh::addGroup(PrimitiveGroupType prim,
               unsigned int materialIndex,
               std::vector<Index32>&& indices)
{
    PrimitiveGroup g;
    g.indices = std::move(indices);
    g.prim = prim;
    g.materialIndex = materialIndex;

    return addGroup(std::move(g));
}


unsigned int
Mesh::getGroupCount() const
{
    return groups.size();
}


void
Mesh::clearGroups()
{
    groups.clear();
}


const std::string&
Mesh::getName() const
{
    return name;
}


void
Mesh::setName(std::string&& _name)
{
    name = std::move(_name);
}


void
Mesh::remapIndices(const std::vector<Index32>& indexMap)
{
    for (auto& group : groups)
    {
        for (auto& index : group.indices)
        {
            index = indexMap[index];
        }
    }
}


void
Mesh::remapMaterials(const std::vector<unsigned int>& materialMap)
{
    for (auto& group : groups)
        group.materialIndex = materialMap[group.materialIndex];
}


void
Mesh::aggregateByMaterial()
{
    std::sort(groups.begin(), groups.end(),
              [](const PrimitiveGroup& g0, const PrimitiveGroup& g1)
              {
                  return g0.materialIndex < g1.materialIndex;
              });
    mergePrimitiveGroups();
}


void
Mesh::mergePrimitiveGroups()
{
    if (groups.size() < 2)
        return;

    std::vector<PrimitiveGroup> newGroups;
    for (size_t i = 0; i < groups.size(); i++)
    {
        auto &g = groups[i];

        if (g.prim == PrimitiveGroupType::TriStrip)
        {
            std::vector<Index32> newIndices;
            newIndices.reserve(g.indices.size() * 2);
            for (size_t j = 0, e = g.indices.size() - 2; j < e; j++)
            {
                auto x = g.indices[j + 0];
                auto y = g.indices[j + 1];
                auto z = g.indices[j + 2];
                // skip degenerated triangles
                if (x == y || y == z || z == x)
                    continue;
                if ((j & 1) != 0) // FIXME: CCW hardcoded
                    std::swap(y, z);
                newIndices.push_back(x);
                newIndices.push_back(y);
                newIndices.push_back(z);
            }
            g.indices = std::move(newIndices);
            g.prim = PrimitiveGroupType::TriList;
        }

        if (i == 0 || g.prim != PrimitiveGroupType::TriList)
        {
            newGroups.push_back(std::move(g));
        }
        else
        {
            auto &p = newGroups.back();
            if (p.prim != g.prim || p.materialIndex != g.materialIndex)
            {
                newGroups.push_back(std::move(g));
            }
            else
            {
                p.indices.reserve(p.indices.size() + g.indices.size());
                p.indices.insert(p.indices.end(), g.indices.begin(), g.indices.end());
            }
        }
    }
    GetLogger()->info("Optimized mesh groups: had {} groups, now: {} of them.\n", groups.size(), newGroups.size());
    groups = std::move(newGroups);
}

void
Mesh::optimize()
{
#ifdef HAVE_MESHOPTIMIZER
    if (groups.size() > 1)
        return;

    auto &g = groups.front();

    meshopt_optimizeVertexCache(g.indices.data(), g.indices.data(), g.indices.size(), nVertices);
    meshopt_optimizeOverdraw(g.indices.data(), g.indices.data(), g.indices.size(), reinterpret_cast<float*>(vertices.data()), nVertices, vertexDesc.strideBytes, 1.05f);
    meshopt_optimizeVertexFetch(vertices.data(), g.indices.data(), g.indices.size(), vertices.data(), nVertices, vertexDesc.strideBytes);
#endif
}

void
Mesh::rebuildIndexMetadata()
{
    int offset = 0;
    for (auto &g : groups)
    {
        g.indicesOffset = offset;
        g.indicesCount = static_cast<int>(g.indices.size());
        offset += g.indicesCount;
    }
    nTotalIndices = offset;
}

bool
Mesh::pick(const Eigen::Vector3d& rayOrigin, const Eigen::Vector3d& rayDirection, PickResult* result) const
{
    double maxDistance = 1.0e30;
    double closest = maxDistance;

    // Pick will automatically fail without vertex positions--no reasonable
    // mesh should lack these.
    if (vertexDesc.getAttribute(VertexAttributeSemantic::Position).semantic != VertexAttributeSemantic::Position ||
        vertexDesc.getAttribute(VertexAttributeSemantic::Position).format != VertexAttributeFormat::Float3)
    {
        return false;
    }

    unsigned int stride = vertexDesc.strideBytes / sizeof(VWord);
    unsigned int posOffset = vertexDesc.getAttribute(VertexAttributeSemantic::Position).offsetWords;
    const VWord* vdata = vertices.data();

    // Iterate over all primitive groups in the mesh
    for (const auto& group : groups)
    {
        PrimitiveGroupType primType = group.prim;
        Index32 nIndices = group.indices.size();

        // Only attempt to compute the intersection of the ray with triangle
        // groups.
        if ((primType == PrimitiveGroupType::TriList
             || primType == PrimitiveGroupType::TriStrip
             || primType == PrimitiveGroupType::TriFan) &&
            (nIndices >= 3) &&
            !(primType == PrimitiveGroupType::TriList && nIndices % 3 != 0))
        {
            unsigned int primitiveIndex = 0;
            Index32 index = 0;
            Index32 i0 = group.indices[0];
            Index32 i1 = group.indices[1];
            Index32 i2 = group.indices[2];

            // Iterate over the triangles in the primitive group
            do
            {
                // Get the triangle vertices v0, v1, and v2
                float fv[3];
                std::memcpy(fv, vdata + i0 * stride + posOffset, sizeof(float) * 3);
                Eigen::Vector3d v0 = Eigen::Map<Eigen::Vector3f>(fv).cast<double>();
                std::memcpy(fv, vdata + i1 * stride + posOffset, sizeof(float) * 3);
                Eigen::Vector3d v1 = Eigen::Map<Eigen::Vector3f>(fv).cast<double>();
                std::memcpy(fv, vdata + i2 * stride + posOffset, sizeof(float) * 3);
                Eigen::Vector3d v2 = Eigen::Map<Eigen::Vector3f>(fv).cast<double>();

                // Compute the edge vectors e0 and e1, and the normal n
                Eigen::Vector3d e0 = v1 - v0;
                Eigen::Vector3d e1 = v2 - v0;
                Eigen::Vector3d n = e0.cross(e1);

                // c is the cosine of the angle between the ray and triangle normal
                double c = n.dot(rayDirection);

                // If the ray is parallel to the triangle, it either misses the
                // triangle completely, or is contained in the triangle's plane.
                // If it's contained in the plane, we'll still call it a miss.
                if (c != 0.0)
                {
                    double t = (n.dot(v0 - rayOrigin)) / c;
                    if (t < closest && t > 0.0)
                    {
                        double m00 = e0.dot(e0);
                        double m01 = e0.dot(e1);
                        double m10 = e1.dot(e0);
                        double m11 = e1.dot(e1);
                        double det = m00 * m11 - m01 * m10;
                        if (det != 0.0)
                        {
                            Eigen::Vector3d p = rayOrigin + rayDirection * t;
                            Eigen::Vector3d q = p - v0;
                            double q0 = e0.dot(q);
                            double q1 = e1.dot(q);
                            double d = 1.0 / det;
                            double s0 = (m11 * q0 - m01 * q1) * d;
                            double s1 = (m00 * q1 - m10 * q0) * d;
                            if (s0 >= 0.0 && s1 >= 0.0 && s0 + s1 <= 1.0)
                            {
                                closest = t;
                                if (result)
                                {
                                    result->group = &group;
                                    result->primitiveIndex = primitiveIndex;
                                    result->distance = closest;
                                }
                            }
                        }
                    }
                }

                // Get the indices for the next triangle
                if (primType == PrimitiveGroupType::TriList)
                {
                    index += 3;
                    if (index < nIndices)
                    {
                        i0 = group.indices[index + 0];
                        i1 = group.indices[index + 1];
                        i2 = group.indices[index + 2];
                    }
                }
                else if (primType == PrimitiveGroupType::TriStrip)
                {
                    index += 1;
                    if (index < nIndices)
                    {
                        i0 = i1;
                        i1 = i2;
                        i2 = group.indices[index];
                        // TODO: alternate orientation of triangles in a strip
                    }
                }
                else // primType == TriFan
                {
                    index += 1;
                    if (index < nIndices)
                    {
                        index += 1;
                        i1 = i2;
                        i2 = group.indices[index];
                    }
                }

                primitiveIndex++;

            } while (index < nIndices);
        }
    }

    return closest != maxDistance;
}


bool
Mesh::pick(const Eigen::Vector3d& rayOrigin, const Eigen::Vector3d& rayDirection, double& distance) const
{
    PickResult result;
    bool hit = pick(rayOrigin, rayDirection, &result);
    if (hit)
    {
        distance = result.distance;
    }

    return hit;
}


Eigen::AlignedBox<float, 3>
Mesh::getBoundingBox() const
{
    Eigen::AlignedBox<float, 3> bbox;

    // Return an empty box if there's no position info
    if (vertexDesc.getAttribute(VertexAttributeSemantic::Position).format != VertexAttributeFormat::Float3)
        return bbox;

    const VWord* vdata = vertices.data() + vertexDesc.getAttribute(VertexAttributeSemantic::Position).offsetWords;

    unsigned int stride = vertexDesc.strideBytes / sizeof(VWord);
    if (vertexDesc.getAttribute(VertexAttributeSemantic::PointSize).format == VertexAttributeFormat::Float1)
    {
        // Handle bounding box calculation for point sprites. Unlike other
        // primitives, point sprite vertices have a non-zero size.
        int pointSizeOffset = (int) vertexDesc.getAttribute(VertexAttributeSemantic::PointSize).offsetWords -
            (int) vertexDesc.getAttribute(VertexAttributeSemantic::Position).offsetWords;

        for (unsigned int i = 0; i < nVertices; i++, vdata += stride)
        {
            float fv[3];
            std::memcpy(fv, vdata, sizeof(float) * 3);
            Eigen::Vector3f center = Eigen::Map<Eigen::Vector3f>(fv);
            float pointSize;
            std::memcpy(&pointSize, vdata + pointSizeOffset, sizeof(float));
            Eigen::Vector3f offsetVec = Eigen::Vector3f::Constant(pointSize);

            Eigen::AlignedBox<float, 3> pointbox(center - offsetVec, center + offsetVec);
            bbox.extend(pointbox);
        }
    }
    else
    {
        for (unsigned int i = 0; i < nVertices; i++, vdata += stride)
        {
            float fv[3];
            std::memcpy(fv, vdata, sizeof(float) * 3);
            bbox.extend(Eigen::Map<Eigen::Vector3f>(fv));
        }
    }

    return bbox;
}


void
Mesh::transform(const Eigen::Vector3f& translation, float scale)
{
    if (vertexDesc.getAttribute(VertexAttributeSemantic::Position).format != VertexAttributeFormat::Float3)
        return;

    VWord* vdata = vertices.data() + vertexDesc.getAttribute(VertexAttributeSemantic::Position).offsetWords;
    unsigned int i;

    unsigned int stride = vertexDesc.strideBytes / sizeof(VWord);

    // Scale and translate the vertex positions
    for (i = 0; i < nVertices; i++, vdata += stride)
    {
        float fv[3];
        std::memcpy(fv, vdata, sizeof(float) * 3);
        const Eigen::Vector3f tv = (Eigen::Map<Eigen::Vector3f>(fv) + translation) * scale;
        std::memcpy(vdata, tv.data(), sizeof(float) * 3);
    }

    // Point sizes need to be scaled as well
    if (vertexDesc.getAttribute(VertexAttributeSemantic::PointSize).format == VertexAttributeFormat::Float1)
    {
        vdata = vertices.data() + vertexDesc.getAttribute(VertexAttributeSemantic::PointSize).offsetWords;
        for (i = 0; i < nVertices; i++, vdata += stride)
        {
            float f;
            std::memcpy(&f, vdata, sizeof(float));
            f *= scale;
            std::memcpy(vdata, &f, sizeof(float));
        }
    }
}


unsigned int
Mesh::getPrimitiveCount() const
{
    unsigned int count = 0;

    for (const auto& group : groups)
        count += group.getPrimitiveCount();

    return count;
}

void
Mesh::merge(const Mesh &other)
{
    auto &ti = groups.front().indices;
    const auto &oi = other.groups.front().indices;

    ti.reserve(ti.size() + oi.size());
    for (auto i : oi)
        ti.push_back(i + nVertices);

    vertices.reserve(vertices.size() + other.vertices.size());
    vertices.insert(vertices.end(), other.vertices.begin(), other.vertices.end());

    nVertices += other.nVertices;
}

bool
Mesh::canMerge(const Mesh &other, const std::vector<Material> &materials) const
{
    if (getGroupCount() != 1 || other.getGroupCount() != 1)
        return false;

    const auto &tg = groups.front();
    const auto &og = other.groups.front();

    if (tg.prim != PrimitiveGroupType::TriList)
        return false;

    if (std::tie(tg.materialIndex, tg.prim, vertexDesc.strideBytes) !=
        std::tie(og.materialIndex, og.prim, other.vertexDesc.strideBytes))
        return false;

    if (!isOpaqueMaterial(materials[tg.materialIndex]) || !isOpaqueMaterial(materials[og.materialIndex]))
        return false;

    for (auto i = VertexAttributeSemantic::Position;
         i < VertexAttributeSemantic::SemanticMax;
         i = static_cast<VertexAttributeSemantic>(1 + static_cast<uint16_t>(i)))
    {
        auto &ta = vertexDesc.getAttribute(i);
        auto &oa = other.vertexDesc.getAttribute(i);
        if (ta.format != oa.format || ta.offsetWords != oa.offsetWords)
            return false;
    }

    return true;
}

} // end namespace cmod
