// mesh.cpp
//
// Copyright (C) 2004-2010, the Celestia Development Team
// Original version by Chris Laurel <claurel@gmail.com>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include "mesh.h"
#include <cassert>
#include <iostream>
#include <algorithm>
#include <Eigen/Core>
#include <Eigen/Geometry>

using namespace cmod;
using namespace Eigen;
using namespace std;


static size_t VertexAttributeFormatSizes[Mesh::FormatMax] =
{
     4,  // Float1
     8,  // Float2
     12, // Float3
     16, // Float4,
     4,  // UByte4
};


Mesh::VertexDescription::VertexDescription(unsigned int _stride,
                                           unsigned int _nAttributes,
                                           VertexAttribute* _attributes) :
            stride(_stride),
            nAttributes(_nAttributes),
            attributes(NULL)
{
    if (nAttributes != 0)
    {
        attributes = new VertexAttribute[nAttributes];
        for (unsigned int i = 0; i < nAttributes; i++)
            attributes[i] = _attributes[i];
        buildSemanticMap();
    }
}


Mesh::VertexDescription::VertexDescription(const VertexDescription& desc) :
    stride(desc.stride),
    nAttributes(desc.nAttributes),
    attributes(NULL)
{
    if (nAttributes != 0)
    {
        attributes = new VertexAttribute[nAttributes];
        for (unsigned int i = 0; i < nAttributes; i++)
            attributes[i] = desc.attributes[i];
        buildSemanticMap();
    }
}


Mesh::VertexDescription&
Mesh::VertexDescription::operator=(const Mesh::VertexDescription& desc)
{
    if (nAttributes < desc.nAttributes)
    {
        if (attributes != NULL)
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
Mesh::VertexDescription::validate() const
{
    for (unsigned int i = 0; i < nAttributes; i++)
    {
        VertexAttribute& attr = attributes[i];

        // Validate the attribute
        if (attr.semantic >= SemanticMax || attr.format >= FormatMax)
            return false;
        if (attr.offset % 4 != 0)
            return false;
        if (attr.offset + VertexAttributeFormatSizes[attr.format] > stride)
            return false;
        // TODO: check for repetition of attributes
        // if (vertexAttributeMap[attr->semantic].format != InvalidFormat)
        //   return false;
    }

    return true;
}


Mesh::VertexDescription::~VertexDescription()
{
    delete[] attributes;
}


void
Mesh::VertexDescription::buildSemanticMap()
{
    for (unsigned int i = 0; i < nAttributes; i++)
        semanticMap[attributes[i].semantic] = attributes[i];
}


void
Mesh::VertexDescription::clearSemanticMap()
{
    for (unsigned int i = 0; i < SemanticMax; i++)
        semanticMap[i] = VertexAttribute();
}


Mesh::PrimitiveGroup::PrimitiveGroup()
{
}


Mesh::PrimitiveGroup::~PrimitiveGroup()
{
    // TODO: probably should free index list; need to sort out
    // ownership issues.
}


unsigned int
Mesh::PrimitiveGroup::getPrimitiveCount() const
{
    switch (prim)
    {
    case TriList:
        return nIndices / 3;
    case TriStrip:
    case TriFan:
        return nIndices - 2;
    case LineList:
        return nIndices / 2;
    case LineStrip:
        return nIndices - 2;
    case PointList:
    case SpriteList:
        return nIndices;
    default:
        // Invalid value
        return 0;
    }
}


Mesh::Mesh() :
    vertexDesc(0, 0, NULL),
    nVertices(0),
    vertices(NULL),
    vbResource(0)
{
}


Mesh::~Mesh()
{
    for (vector<PrimitiveGroup*>::iterator iter = groups.begin();
         iter != groups.end(); iter++)
    {
        delete *iter;
    }

    // TODO: this is just to cast away void* and shut up GCC warnings;
    // should probably be static_cast<VertexList::VertexPart*>
    if (vertices != NULL)
    {
        delete[] static_cast<char*>(vertices);
    }

    if (vbResource)
    {
        delete vbResource;
    }
}


void
Mesh::setVertices(unsigned int _nVertices, void* vertexData)
{
    if (vertexData == vertices)
    {
        return;
    }

    // TODO: this is just to cast away void* and shut up GCC warnings;
    // should probably be static_cast<VertexList::VertexPart*>
    if (vertices != NULL)
    {
        delete[] static_cast<char*>(vertices);
    }

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


const Mesh::VertexDescription& Mesh::getVertexDescription() const
{
    return vertexDesc;
}


const Mesh::PrimitiveGroup*
Mesh::getGroup(unsigned int index) const
{
    if (index >= groups.size())
        return NULL;
    else
        return groups[index];
}


Mesh::PrimitiveGroup*
Mesh::getGroup(unsigned int index)
{
    if (index >= groups.size())
        return NULL;
    else
        return groups[index];
}


unsigned int
Mesh::addGroup(PrimitiveGroup* group)
{
    groups.push_back(group);
    return groups.size();
}


unsigned int
Mesh::addGroup(PrimitiveGroupType prim,
               unsigned int materialIndex,
               unsigned int nIndices,
               index32* indices)
{
    PrimitiveGroup* g = new PrimitiveGroup();
    g->prim = prim;
    g->materialIndex = materialIndex;
    g->nIndices = nIndices;
    g->indices = indices;

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
    for (vector<PrimitiveGroup*>::iterator iter = groups.begin();
         iter != groups.end(); iter++)
    {
        delete *iter;
    }

    groups.clear();
}


const string&
Mesh::getName() const
{
    return name;
}


void
Mesh::setName(const string& _name)
{
    name = _name;
}


void
Mesh::remapIndices(const vector<index32>& indexMap)
{
    for (vector<PrimitiveGroup*>::iterator iter = groups.begin();
         iter != groups.end(); iter++)
    {
        PrimitiveGroup* group = *iter;
        for (index32 i = 0; i < group->nIndices; i++)
        {
            group->indices[i] = indexMap[group->indices[i]];
        }
    }
}


void
Mesh::remapMaterials(const vector<unsigned int>& materialMap)
{
    for (vector<PrimitiveGroup*>::iterator iter = groups.begin();
         iter != groups.end(); iter++)
    {
        (*iter)->materialIndex = materialMap[(*iter)->materialIndex];
    }
}


class PrimitiveGroupComparator : public std::binary_function<const Mesh::PrimitiveGroup*, const Mesh::PrimitiveGroup*, bool>
{
public:
    bool operator()(const Mesh::PrimitiveGroup* g0, const Mesh::PrimitiveGroup* g1) const
    {
        return g0->materialIndex < g1->materialIndex;
    }

private:
    int unused;
};


void
Mesh::aggregateByMaterial()
{
    sort(groups.begin(), groups.end(), PrimitiveGroupComparator());
}


bool
Mesh::pick(const Vector3d& rayOrigin, const Vector3d& rayDirection, PickResult* result) const
{
    double maxDistance = 1.0e30;
    double closest = maxDistance;

    // Pick will automatically fail without vertex positions--no reasonable
    // mesh should lack these.
    if (vertexDesc.getAttribute(Position).semantic != Position ||
        vertexDesc.getAttribute(Position).format != Float3)
    {
        return false;
    }

    unsigned int posOffset = vertexDesc.getAttribute(Position).offset;
    char* vdata = reinterpret_cast<char*>(vertices);

    // Iterate over all primitive groups in the mesh
    for (vector<PrimitiveGroup*>::const_iterator iter = groups.begin();
         iter != groups.end(); iter++)
    {
        Mesh::PrimitiveGroupType primType = (*iter)->prim;
        index32 nIndices = (*iter)->nIndices;

        // Only attempt to compute the intersection of the ray with triangle
        // groups.
        if ((primType == TriList || primType == TriStrip || primType == TriFan) &&
            (nIndices >= 3) &&
            !(primType == TriList && nIndices % 3 != 0))
        {
            unsigned int primitiveIndex = 0;
            index32 index = 0;
            index32 i0 = (*iter)->indices[0];
            index32 i1 = (*iter)->indices[1];
            index32 i2 = (*iter)->indices[2];

            // Iterate over the triangles in the primitive group
            do
            {
                // Get the triangle vertices v0, v1, and v2
                Vector3d v0 = Map<Vector3f>(reinterpret_cast<float*>(vdata + i0 * vertexDesc.stride + posOffset)).cast<double>();
                Vector3d v1 = Map<Vector3f>(reinterpret_cast<float*>(vdata + i1 * vertexDesc.stride + posOffset)).cast<double>();
                Vector3d v2 = Map<Vector3f>(reinterpret_cast<float*>(vdata + i2 * vertexDesc.stride + posOffset)).cast<double>();

                // Compute the edge vectors e0 and e1, and the normal n
                Vector3d e0 = v1 - v0;
                Vector3d e1 = v2 - v0;
                Vector3d n = e0.cross(e1);

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
                            Vector3d p = rayOrigin + rayDirection * t;
                            Vector3d q = p - v0;
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
                                    result->group = *iter;
                                    result->primitiveIndex = primitiveIndex;
                                    result->distance = closest;
                                }
                            }
                        }
                    }
                }

                // Get the indices for the next triangle
                if (primType == TriList)
                {
                    index += 3;
                    if (index < nIndices)
                    {
                        i0 = (*iter)->indices[index + 0];
                        i1 = (*iter)->indices[index + 1];
                        i2 = (*iter)->indices[index + 2];
                    }
                }
                else if (primType == TriStrip)
                {
                    index += 1;
                    if (index < nIndices)
                    {
                        i0 = i1;
                        i1 = i2;
                        i2 = (*iter)->indices[index];
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
                        i2 = (*iter)->indices[index];
                    }
                }

                primitiveIndex++;

            } while (index < nIndices);
        }
    }

    return closest != maxDistance;
}


bool
Mesh::pick(const Vector3d& rayOrigin, const Vector3d& rayDirection, double& distance) const
{
    PickResult result;
    bool hit = pick(rayOrigin, rayDirection, &result);
    if (hit)
    {
        distance = result.distance;
    }

    return hit;
}


AlignedBox<float, 3>
Mesh::getBoundingBox() const
{
    AlignedBox<float, 3> bbox;

    // Return an empty box if there's no position info
    if (vertexDesc.getAttribute(Position).format != Float3)
        return bbox;

    char* vdata = reinterpret_cast<char*>(vertices) + vertexDesc.getAttribute(Position).offset;

    if (vertexDesc.getAttribute(PointSize).format == Float1)
    {
        // Handle bounding box calculation for point sprites. Unlike other
        // primitives, point sprite vertices have a non-zero size.
        int pointSizeOffset = (int) vertexDesc.getAttribute(PointSize).offset -
            (int) vertexDesc.getAttribute(Position).offset;
        
        for (unsigned int i = 0; i < nVertices; i++, vdata += vertexDesc.stride)
        {
            Vector3f center = Map<Vector3f>(reinterpret_cast<float*>(vdata));
            float pointSize = (reinterpret_cast<float*>(vdata + pointSizeOffset))[0];
            Vector3f offsetVec = Vector3f::Constant(pointSize);

            AlignedBox<float, 3> pointbox(center - offsetVec, center + offsetVec);
            bbox.extend(pointbox);
        }
    }
    else
    {
        for (unsigned int i = 0; i < nVertices; i++, vdata += vertexDesc.stride)
            bbox.extend(Map<Vector3f>(reinterpret_cast<float*>(vdata)));
    }

    return bbox;
}


void
Mesh::transform(const Vector3f& translation, float scale)
{
    if (vertexDesc.getAttribute(Position).format != Float3)
        return;

    char* vdata = reinterpret_cast<char*>(vertices) + vertexDesc.getAttribute(Position).offset;
    unsigned int i;

    // Scale and translate the vertex positions
    for (i = 0; i < nVertices; i++, vdata += vertexDesc.stride)
    {
        const Vector3f tv = (Map<Vector3f>(reinterpret_cast<float*>(vdata)) + translation) * scale;
        Map<Vector3f>(reinterpret_cast<float*>(vdata)) = tv;
    }

    // Point sizes need to be scaled as well
    if (vertexDesc.getAttribute(PointSize).format == Float1)
    {
        vdata = reinterpret_cast<char*>(vertices) + vertexDesc.getAttribute(PointSize).offset;
        for (i = 0; i < nVertices; i++, vdata += vertexDesc.stride)
            reinterpret_cast<float*>(vdata)[0] *= scale;
    }
}


unsigned int
Mesh::getPrimitiveCount() const
{
    unsigned int count = 0;

    for (vector<PrimitiveGroup*>::const_iterator iter = groups.begin();
         iter != groups.end(); iter++)
    {
        count += (*iter)->getPrimitiveCount();
    }

    return count;
}



Mesh::PrimitiveGroupType
Mesh::parsePrimitiveGroupType(const string& name)
{
    if (name == "trilist")
        return TriList;
    else if (name == "tristrip")
        return TriStrip;
    else if (name == "trifan")
        return TriFan;
    else if (name == "linelist")
        return LineList;
    else if (name == "linestrip")
        return LineStrip;
    else if (name == "points")
        return PointList;
    else if (name == "sprites")
        return SpriteList;
    else
        return InvalidPrimitiveGroupType;
}


Mesh::VertexAttributeSemantic
Mesh::parseVertexAttributeSemantic(const string& name)
{
    if (name == "position")
        return Position;
    else if (name == "normal")
        return Normal;
    else if (name == "color0")
        return Color0;
    else if (name == "color1")
        return Color1;
    else if (name == "tangent")
        return Tangent;
    else if (name == "texcoord0")
        return Texture0;
    else if (name == "texcoord1")
        return Texture1;
    else if (name == "texcoord2")
        return Texture2;
    else if (name == "texcoord3")
        return Texture3;
    else if (name == "pointsize")
        return PointSize;
    else
        return InvalidSemantic;
}


Mesh::VertexAttributeFormat
Mesh::parseVertexAttributeFormat(const string& name)
{
    if (name == "f1")
        return Float1;
    else if (name == "f2")
        return Float2;
    else if (name == "f3")
        return Float3;
    else if (name == "f4")
        return Float4;
    else if (name == "ub4")
        return UByte4;
    else
        return InvalidFormat;
}


Material::TextureSemantic
Mesh::parseTextureSemantic(const string& name)
{
    if (name == "texture0")
        return Material::DiffuseMap;
    else if (name == "normalmap")
        return Material::NormalMap;
    else if (name == "specularmap")
        return Material::SpecularMap;
    else if (name == "emissivemap")
        return Material::EmissiveMap;
    else
        return Material::InvalidTextureSemantic;
}


unsigned int
Mesh::getVertexAttributeSize(VertexAttributeFormat fmt)
{
    switch (fmt)
    {
    case Float1:
    case UByte4:
        return 4;
    case Float2:
        return 8;
    case Float3:
        return 12;
    case Float4:
        return 16;
    default:
        return 0;
    }
}


Mesh::PickResult::PickResult() :
    mesh(NULL),
    group(NULL),
    primitiveIndex(0),
    distance(-1.0)
{
}
