// mesh.cpp
//
// Copyright (C) 2004-2010, the Celestia Development Team
// Original version by Chris Laurel <claurel@gmail.com>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include <array>
#include <cstring>


#include "mesh.h"


namespace cmod
{

VertexDescription::VertexDescription(unsigned int _stride,
                                     unsigned int _nAttributes,
                                     VertexAttribute* _attributes) :
    stride(_stride),
    nAttributes(_nAttributes),
    attributes(nullptr)
{
    if (nAttributes != 0)
    {
        attributes = new VertexAttribute[nAttributes];
        for (unsigned int i = 0; i < nAttributes; i++)
            attributes[i] = _attributes[i];
        buildSemanticMap();
    }
}


VertexDescription::VertexDescription(const VertexDescription& desc) :
    stride(desc.stride),
    nAttributes(desc.nAttributes),
    attributes(nullptr)
{
    if (nAttributes != 0)
    {
        attributes = new VertexAttribute[nAttributes];
        for (unsigned int i = 0; i < nAttributes; i++)
            attributes[i] = desc.attributes[i];
        buildSemanticMap();
    }
}


VertexDescription&
VertexDescription::operator=(const VertexDescription& desc)
{
    if (this == &desc)
        return *this;

    if (nAttributes < desc.nAttributes)
    {
        delete[] attributes;
        attributes = new VertexAttribute[desc.nAttributes];
    }

    nAttributes = desc.nAttributes;
    stride = desc.stride;
    for (unsigned int i = 0; i < nAttributes; i++)
        attributes[i] = desc.attributes[i];
    clearSemanticMap();
    buildSemanticMap();

    return *this;
}


// TODO: This should be called in the constructor; we should start using
// exceptions in Celestia.
bool
VertexDescription::validate() const
{
    for (unsigned int i = 0; i < nAttributes; i++)
    {
        VertexAttribute& attr = attributes[i];

        // Validate the attribute
        if (attr.offset % 4 != 0 || attr.offset + VertexAttribute::getFormatSize(attr.format) > stride)
            return false;
        // TODO: check for repetition of attributes
        // if (vertexAttributeMap[attr->semantic].format != InvalidFormat)
        //   return false;
    }

    return true;
}

VertexDescription VertexDescription::appendingAttributes(const VertexAttribute* newAttributes, int count) const
{
    unsigned int totalAttributeCount = nAttributes + count;
    VertexAttribute* allAttributes = new VertexAttribute[totalAttributeCount];
    std::memcpy(allAttributes, attributes, sizeof(VertexAttribute) * nAttributes);
    unsigned int newStride = stride;
    for (int i = 0; i < count; ++i)
    {
        VertexAttribute attribute = newAttributes[i];
        allAttributes[nAttributes + i] = attribute;
        newStride += VertexAttribute::getFormatSize(attribute.format);
    }
    std::memcpy(allAttributes + nAttributes, newAttributes, sizeof(VertexAttribute) * count);
    VertexDescription desc = VertexDescription(newStride, totalAttributeCount, allAttributes);
    delete[] allAttributes;
    return desc;
}

VertexDescription::~VertexDescription()
{
    delete[] attributes;
}


void
VertexDescription::buildSemanticMap()
{
    for (unsigned int i = 0; i < nAttributes; i++)
        semanticMap[static_cast<std::size_t>(attributes[i].semantic)] = attributes[i];
}


void
VertexDescription::clearSemanticMap()
{
    for (auto& i : semanticMap)
        i = VertexAttribute();
}


unsigned int
PrimitiveGroup::getPrimitiveCount() const
{
    switch (prim)
    {
    case PrimitiveGroupType::TriList:
        return nIndices / 3;
    case PrimitiveGroupType::TriStrip:
    case PrimitiveGroupType::TriFan:
        return nIndices - 2;
    case PrimitiveGroupType::LineList:
        return nIndices / 2;
    case PrimitiveGroupType::LineStrip:
        return nIndices - 2;
    case PrimitiveGroupType::PointList:
    case PrimitiveGroupType::SpriteList:
        return nIndices;
    default:
        // Invalid value
        return 0;
    }
}


Mesh::~Mesh()
{
    for (const auto group : groups)
        delete group;

    // TODO: this is just to cast away void* and shut up GCC warnings;
    // should probably be static_cast<VertexList::VertexPart*>
    delete[] static_cast<char*>(vertices);
}


void
Mesh::setVertices(unsigned int _nVertices, void* vertexData)
{
    if (vertexData == vertices)
        return;

    // TODO: this is just to cast away void* and shut up GCC warnings;
    // should probably be static_cast<VertexList::VertexPart*>
    delete[] static_cast<char*>(vertices);

    nVertices = _nVertices;
    vertices = vertexData;
}


bool
Mesh::setVertexDescription(const VertexDescription& desc)
{
    if (!desc.validate())
        return false;

    vertexDesc = desc;

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

    return groups[index];
}


PrimitiveGroup*
Mesh::getGroup(unsigned int index)
{
    if (index >= groups.size())
        return nullptr;

    return groups[index];
}


unsigned int
Mesh::addGroup(PrimitiveGroup* group)
{
    groups.push_back(group);
    return groups.size();
}


PrimitiveGroup*
Mesh::createLinePrimitiveGroup(bool lineStrip, unsigned int nIndices, index32* indices)
{
    // Transform LINE_STRIP/LINES to triangle vertices
    int transformedVertCount = lineStrip ? (nIndices - 1) * 4 : nIndices * 2;
    // Get information of the position attributes
    auto positionAttributes = vertexDesc.getAttribute(VertexAttributeSemantic::Position);
    int positionSize = VertexAttribute::getFormatSize(positionAttributes.format);
    int positionOffset = positionAttributes.offset;

    int originalStride = vertexDesc.stride;
    // Add another position (next line end), and scale factor
    // ORIGINAL ATTRIBUTES | NextVCoordAttributeIndex | ScaleFactorAttributeIndex
    int stride = originalStride + positionSize + sizeof(float);

    // Get line count
    int lineCount = lineStrip ? nIndices - 1 : nIndices / 2;
    int lineIndexCount = 6 * lineCount;
    int lineVertexCount = 4 * lineCount;

    // Create buffer to hold the transformed vertices/indices
    unsigned char* data = new unsigned char[stride * lineVertexCount];
    index32* newIndices = new index32[lineIndexCount];

    unsigned char* originalData = (unsigned char *)vertices;
    auto ptr = data;
    for (int i = 0; i < lineCount; ++i)
    {
        int thisIndex = indices[lineStrip ? i : i * 2];
        int nextIndex = indices[lineStrip ? i + 1 : i * 2 + 1];

        unsigned char* origThisVertLoc = originalData + thisIndex * originalStride;
        unsigned char* origNextVertLoc = originalData + nextIndex * originalStride;
        float *ff = (float *)origThisVertLoc;
        float *ffn = (float *)origNextVertLoc;

        // Fill the info for the 4 vertices
        std::memcpy(ptr, origThisVertLoc, originalStride);
        std::memcpy(ptr + originalStride, origNextVertLoc + positionOffset, positionSize);
        *(float *)&ptr[originalStride + positionSize] = -0.5f;
        ptr += stride;

        std::memcpy(ptr, origThisVertLoc, originalStride);
        std::memcpy(ptr + originalStride, origNextVertLoc + positionOffset, positionSize);
        *(float *)&ptr[originalStride + positionSize] = 0.5f;
        ptr += stride;

        std::memcpy(ptr, origNextVertLoc, originalStride);
        std::memcpy(ptr + originalStride, origThisVertLoc + positionOffset, positionSize);
        *(float *)&ptr[originalStride + positionSize] = -0.5f;
        ptr += stride;

        std::memcpy(ptr, origNextVertLoc, originalStride);
        std::memcpy(ptr + originalStride, origThisVertLoc + positionOffset, positionSize);
        *(float *)&ptr[originalStride + positionSize] = 0.5f;
        ptr += stride;

        int lineIndex = 6 * i;
        index32 newIndex = 4 * i;

        // Fill info for the 6 indices
        newIndices[lineIndex] = newIndex;
        newIndices[lineIndex + 1] = newIndex + 1;
        newIndices[lineIndex + 2] = newIndex + 2;
        newIndices[lineIndex + 3] = newIndex + 2;
        newIndices[lineIndex + 4] = newIndex + 3;
        newIndices[lineIndex + 5] = newIndex;
    }

    std::array<VertexAttribute, 2> newAttributes = {
        VertexAttribute(VertexAttributeSemantic::NextPosition, positionAttributes.format, originalStride),
        VertexAttribute(VertexAttributeSemantic::ScaleFactor, VertexAttributeFormat::Float1, originalStride + positionSize),
    };
    auto* g = new PrimitiveGroup();
    g->vertexOverride = data;
    g->vertexCountOverride = lineVertexCount;
    g->vertexDescriptionOverride = vertexDesc.appendingAttributes(newAttributes.data(), 2);
    g->indicesOverride = newIndices;
    g->nIndicesOverride = lineIndexCount;
    g->primOverride = PrimitiveGroupType::TriList;
    return g;
}


unsigned int
Mesh::addGroup(PrimitiveGroupType prim,
               unsigned int materialIndex,
               unsigned int nIndices,
               index32* indices)
{
    PrimitiveGroup* g;
    if (prim == PrimitiveGroupType::LineStrip || prim == PrimitiveGroupType::LineList)
    {
        g = createLinePrimitiveGroup(prim == PrimitiveGroupType::LineStrip, nIndices, indices);
    }
    else
    {
        g = new PrimitiveGroup;
        g->vertexOverride = nullptr;
        g->vertexCountOverride = 0;
        g->indicesOverride = nullptr;
        g->nIndicesOverride = 0;
        g->primOverride = prim;
    }
    g->nIndices = nIndices;
    g->indices = indices;
    g->prim = prim;
    g->materialIndex = materialIndex;

    return addGroup(g);
}


unsigned int
Mesh::getGroupCount() const
{
    return groups.size();
}


void
Mesh::clearGroups()
{
    for (const auto group : groups)
        delete group;

    groups.clear();
}


const std::string&
Mesh::getName() const
{
    return name;
}


void
Mesh::setName(const std::string& _name)
{
    name = _name;
}


void
Mesh::remapIndices(const std::vector<index32>& indexMap)
{
    for (auto group : groups)
    {
        for (index32 i = 0; i < group->nIndices; i++)
        {
            group->indices[i] = indexMap[group->indices[i]];
        }
    }
}


void
Mesh::remapMaterials(const std::vector<unsigned int>& materialMap)
{
    for (auto group : groups)
        group->materialIndex = materialMap[group->materialIndex];
}


void
Mesh::aggregateByMaterial()
{
    std::sort(groups.begin(), groups.end(),
              [](const PrimitiveGroup* g0, const PrimitiveGroup* g1)
              {
                  return g0->materialIndex < g1->materialIndex;
              });
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

    unsigned int posOffset = vertexDesc.getAttribute(VertexAttributeSemantic::Position).offset;
    auto* vdata = reinterpret_cast<char*>(vertices);

    // Iterate over all primitive groups in the mesh
    for (const auto group : groups)
    {
        PrimitiveGroupType primType = group->prim;
        index32 nIndices = group->nIndices;

        // Only attempt to compute the intersection of the ray with triangle
        // groups.
        if ((primType == PrimitiveGroupType::TriList
             || primType == PrimitiveGroupType::TriStrip
             || primType == PrimitiveGroupType::TriFan) &&
            (nIndices >= 3) &&
            !(primType == PrimitiveGroupType::TriList && nIndices % 3 != 0))
        {
            unsigned int primitiveIndex = 0;
            index32 index = 0;
            index32 i0 = group->indices[0];
            index32 i1 = group->indices[1];
            index32 i2 = group->indices[2];

            // Iterate over the triangles in the primitive group
            do
            {
                // Get the triangle vertices v0, v1, and v2
                Eigen::Vector3d v0 = Eigen::Map<Eigen::Vector3f>(reinterpret_cast<float*>(vdata + i0 * vertexDesc.stride + posOffset)).cast<double>();
                Eigen::Vector3d v1 = Eigen::Map<Eigen::Vector3f>(reinterpret_cast<float*>(vdata + i1 * vertexDesc.stride + posOffset)).cast<double>();
                Eigen::Vector3d v2 = Eigen::Map<Eigen::Vector3f>(reinterpret_cast<float*>(vdata + i2 * vertexDesc.stride + posOffset)).cast<double>();

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
                                    result->group = group;
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
                        i0 = group->indices[index + 0];
                        i1 = group->indices[index + 1];
                        i2 = group->indices[index + 2];
                    }
                }
                else if (primType == PrimitiveGroupType::TriStrip)
                {
                    index += 1;
                    if (index < nIndices)
                    {
                        i0 = i1;
                        i1 = i2;
                        i2 = group->indices[index];
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
                        i2 = group->indices[index];
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

    char* vdata = reinterpret_cast<char*>(vertices) + vertexDesc.getAttribute(VertexAttributeSemantic::Position).offset;

    if (vertexDesc.getAttribute(VertexAttributeSemantic::PointSize).format == VertexAttributeFormat::Float1)
    {
        // Handle bounding box calculation for point sprites. Unlike other
        // primitives, point sprite vertices have a non-zero size.
        int pointSizeOffset = (int) vertexDesc.getAttribute(VertexAttributeSemantic::PointSize).offset -
            (int) vertexDesc.getAttribute(VertexAttributeSemantic::Position).offset;

        for (unsigned int i = 0; i < nVertices; i++, vdata += vertexDesc.stride)
        {
            Eigen::Vector3f center = Eigen::Map<Eigen::Vector3f>(reinterpret_cast<float*>(vdata));
            float pointSize = (reinterpret_cast<float*>(vdata + pointSizeOffset))[0];
            Eigen::Vector3f offsetVec = Eigen::Vector3f::Constant(pointSize);

            Eigen::AlignedBox<float, 3> pointbox(center - offsetVec, center + offsetVec);
            bbox.extend(pointbox);
        }
    }
    else
    {
        for (unsigned int i = 0; i < nVertices; i++, vdata += vertexDesc.stride)
            bbox.extend(Eigen::Map<Eigen::Vector3f>(reinterpret_cast<float*>(vdata)));
    }

    return bbox;
}


void
Mesh::transform(const Eigen::Vector3f& translation, float scale)
{
    if (vertexDesc.getAttribute(VertexAttributeSemantic::Position).format != VertexAttributeFormat::Float3)
        return;

    char* vdata = reinterpret_cast<char*>(vertices) + vertexDesc.getAttribute(VertexAttributeSemantic::Position).offset;
    unsigned int i;

    // Scale and translate the vertex positions
    for (i = 0; i < nVertices; i++, vdata += vertexDesc.stride)
    {
        const Eigen::Vector3f tv = (Eigen::Map<Eigen::Vector3f>(reinterpret_cast<float*>(vdata)) + translation) * scale;
        Eigen::Map<Eigen::Vector3f>(reinterpret_cast<float*>(vdata)) = tv;
    }

    // Scale and translate the overriden vertex values
    for (i = 0; i < getGroupCount(); i++)
    {
        PrimitiveGroup* group = getGroup(i);
        char* vdata = reinterpret_cast<char*>(group->vertexOverride);
        if (!vdata)
            continue;

        auto vertexDesc = group->vertexDescriptionOverride;
        int positionOffset = vertexDesc.getAttribute(VertexAttributeSemantic::Position).offset;
        int nextPositionOffset = vertexDesc.getAttribute(VertexAttributeSemantic::NextPosition).offset;
        for (unsigned int j = 0; j < group->vertexCountOverride; j++, vdata += vertexDesc.stride)
        {
            Eigen::Vector3f tv = (Eigen::Map<Eigen::Vector3f>(reinterpret_cast<float*>(vdata + positionOffset)) + translation) * scale;
            Eigen::Map<Eigen::Vector3f>(reinterpret_cast<float*>(vdata + positionOffset)) = tv;

            tv = (Eigen::Map<Eigen::Vector3f>(reinterpret_cast<float*>(vdata + nextPositionOffset)) + translation) * scale;
            Eigen::Map<Eigen::Vector3f>(reinterpret_cast<float*>(vdata + nextPositionOffset)) = tv;
        }
    }

    // Point sizes need to be scaled as well
    if (vertexDesc.getAttribute(VertexAttributeSemantic::PointSize).format == VertexAttributeFormat::Float1)
    {
        vdata = reinterpret_cast<char*>(vertices) + vertexDesc.getAttribute(VertexAttributeSemantic::PointSize).offset;
        for (i = 0; i < nVertices; i++, vdata += vertexDesc.stride)
            reinterpret_cast<float*>(vdata)[0] *= scale;
    }
}


unsigned int
Mesh::getPrimitiveCount() const
{
    unsigned int count = 0;

    for (const auto group : groups)
        count += group->getPrimitiveCount();

    return count;
}

} // end namespace cmod
